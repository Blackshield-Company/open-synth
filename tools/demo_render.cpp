// Offline demo renderer: plays a short scripted phrase through several factory
// presets using the real Open Synth engine (no audio device, no GUI) and writes
// a 48 kHz / 16-bit stereo WAV.
//
// Usage: demo_render [out.wav]   (run from the repo root so samples/ resolves)
#include "synth_engine_wrapper.h"
#include "synth_engine.h"
#include "preset_data.h"
#include "preset_library_full.h"
#include "sample_player.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

using namespace opensynth;

namespace {

struct Note { double start, length; int note; int velocity; };

struct Segment {
    const char* presetName;
    double seconds;
    std::vector<Note> notes;
};

const PresetData* findPreset(const char* name) {
    for (int i = 0; i < kNumFullPresets; ++i)
        if (std::strcmp(kFullPresets[i].name, name) == 0) return &kFullPresets[i];
    return nullptr;
}

std::vector<Note> chords(const std::vector<std::vector<int>>& progression, double beat, int vel) {
    std::vector<Note> out;
    double t = 0.0;
    for (auto& chord : progression) {
        for (int n : chord) out.push_back({t, beat * 4 - 0.05, n, vel});
        t += beat * 4;
    }
    return out;
}

std::vector<Note> pattern(const std::vector<int>& notes, double step, double gate, int repeats, int vel) {
    std::vector<Note> out;
    double t = 0.0;
    for (int r = 0; r < repeats; ++r)
        for (int n : notes) {
            if (n >= 0) out.push_back({t, step * gate, n, vel});
            t += step;
        }
    return out;
}

} // namespace

int main(int argc, char** argv) {
    const double sr = 48000.0;
    const int block = 256;
    const char* outPath = argc > 1 ? argv[1] : "demo.wav";
    const double beat = 60.0 / 110.0; // 110 BPM

    // Am - F - C - G
    const std::vector<std::vector<int>> prog = {
        {57, 60, 64}, {53, 57, 60}, {48, 55, 64}, {55, 59, 62}};

    std::vector<Segment> segments = {
        {"Sunset Pad", beat * 16 + 0.8, chords(prog, beat, 90)},
        {"Acid Bass Modern", beat * 16 + 0.8,
         pattern({33, 33, 45, 33, 29, 29, 41, 29, 36, 36, 48, 36, 31, 31, 43, 43}, beat, 0.45, 1, 110)},
        {"FM Bell Layered", beat * 16 + 0.8,
         pattern({69, 72, 76, 72, 65, 69, 72, 69, 67, 72, 76, 72, 67, 71, 74, 71}, beat / 2, 0.8, 2, 95)},
        {"Super Saw Soft", beat * 16 + 0.8,
         pattern({76, -1, 74, 72, 74, -1, 72, 69, 72, -1, 71, 67, 71, -1, 69, -1}, beat, 0.9, 1, 100)},
        {"Synth Brass Dark", beat * 16 + 0.8,
         pattern({57, -1, -1, 57, 53, -1, -1, 53, 55, -1, -1, 55, 55, -1, 59, -1}, beat, 0.6, 1, 105)},
        {"Grand Piano", beat * 16 + 2.5, chords(prog, beat, 85)},
    };

    std::vector<float> L, R;
    for (auto& seg : segments) {
        const size_t segStart = L.size();
        const PresetData* preset = findPreset(seg.presetName);
        if (!preset) { fprintf(stderr, "preset not found: %s\n", seg.presetName); return 1; }

        SynthEngineWrapper w;
        w.prepare(sr, block);
        auto* engine = w.getEngine();
        auto sp = std::make_unique<SamplePlayer>();
        sp->prepare(sr);
        engine->setSamplePlayer(std::move(sp));
        applyPresetToEngine(*preset, w);
        engine->getSamplePlayer()->waitForPreload();
        fprintf(stderr, "%-18s zones=%d\n", seg.presetName, engine->getSamplePlayer()->zoneCount());

        // Collect events as absolute sample positions.
        struct Ev { long pos; juce::MidiMessage msg; };
        std::vector<Ev> events;
        const long lead = 10L * block; // let queued preset params drain first
        for (auto& n : seg.notes) {
            events.push_back({lead + (long)(n.start * sr), juce::MidiMessage::noteOn(1, n.note, (juce::uint8)n.velocity)});
            events.push_back({lead + (long)((n.start + n.length) * sr), juce::MidiMessage::noteOff(1, n.note)});
        }

        const long total = lead + (long)(seg.seconds * sr);
        juce::AudioBuffer<float> buf(2, block);
        for (long pos = 0; pos < total; pos += block) {
            juce::MidiBuffer midi;
            for (auto& e : events)
                if (e.pos >= pos && e.pos < pos + block) midi.addEvent(e.msg, (int)(e.pos - pos));
            buf.clear();
            w.render(buf, midi);
            if (pos < lead) continue;
            for (int f = 0; f < block; ++f) {
                L.push_back(buf.getSample(0, f));
                R.push_back(buf.getSample(1, f));
            }
        }
        // Short fade-out so segment boundaries never click.
        const size_t fade = std::min(L.size() - segStart, (size_t)(0.25 * sr));
        for (size_t i = 0; i < fade; ++i) {
            const float g = (float)i / (float)fade;
            L[L.size() - 1 - i] *= g;
            R[R.size() - 1 - i] *= g;
        }
    }

    // Normalise to -1 dBFS peak and check sanity.
    float peak = 0.0f;
    for (size_t i = 0; i < L.size(); ++i) {
        if (!std::isfinite(L[i]) || !std::isfinite(R[i])) { fprintf(stderr, "NaN/Inf in render\n"); return 1; }
        peak = std::max(peak, std::max(std::fabs(L[i]), std::fabs(R[i])));
    }
    if (peak < 1e-4f) { fprintf(stderr, "render is silent\n"); return 1; }
    const float gain = 0.89f / peak;

    const int frames = (int)L.size();
    juce::AudioBuffer<float> out(2, frames);
    for (int i = 0; i < frames; ++i) {
        out.setSample(0, i, L[(size_t)i] * gain);
        out.setSample(1, i, R[(size_t)i] * gain);
    }
    juce::File file = juce::File::getCurrentWorkingDirectory().getChildFile(outPath);
    file.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> os(file.createOutputStream().release());
    if (!os) { fprintf(stderr, "cannot open %s\n", outPath); return 1; }
    std::unique_ptr<juce::AudioFormatWriter> writer(wav.createWriterFor(os.get(), sr, 2, 16, {}, 0));
    if (!writer) return 1;
    os.release();
    writer->writeFromAudioSampleBuffer(out, 0, frames);
    printf("wrote %s: %.1f s, peak %.3f -> normalised\n", outPath, frames / sr, peak);
    return 0;
}
