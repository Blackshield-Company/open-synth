#pragma once
#include <cstdint>
#include <array>
#include <memory>
#include "audio_buffer.h"
#include "oscillator.h"
#include "filter.h"
#include "lfo.h"
#include "voice_allocator.h"
#include "param_queue.h"
#include "arpeggiator.h"
#include "drum_synth.h"
#include "rhythm_pattern_player.h"
#include "fx_engine.h"
#include "synth_part.h"
#include "recorder.h"
#include "sample_player.h"
#include "mpe_voice.h"

namespace opensynth {

// Forward declaration for the LegacyFxProcessor used in slot 0 of the FxEngine.
class LegacyFxProcessor;

class SynthEngine {
public:
    static constexpr int MAX_PARTS = 16;

    SynthEngine(double sampleRate, uint32_t blockSize);
    ~SynthEngine();

    // No copy
    SynthEngine(const SynthEngine&) = delete;
    SynthEngine& operator=(const SynthEngine&) = delete;

    void process(AudioBuffer& output);
    void reset();

    // MIDI (thread-safe via queue)
    void noteOn(int midiNote, float velocity, int channel = 0);
    void noteOff(int midiNote, int channel = 0);

    // ── Performance mode: split / layer / transpose ──
    // Split: notes below splitPoint (on the main channel's part) route to
    //        part 1, which is auto-configured as a plucked Karplus bass.
    // Layer: every note triggers both the channel's part AND part 1, which
    //        mirrors part 0 with +/-8 cent detune (classic "Dual" thickness).
    // Layer wins if both are enabled. Samples (ROMpler) fire on part 0 only.
    void setSplitEnabled(bool e);
    void setSplitPoint(int note) { perfSplitPoint_ = std::clamp(note, 21, 108); }
    void setLayerEnabled(bool e);
    void setTranspose(int semitones) { perfTranspose_ = std::clamp(semitones, -12, 12); }
    bool splitEnabled() const { return perfSplitEnabled_; }
    int splitPoint() const { return perfSplitPoint_; }
    bool layerEnabled() const { return perfLayerEnabled_; }
    int transpose() const { return perfTranspose_; }
    void allNotesOff(int channel = -1);
    // Instantly kills all voices (synth + sample) — MIDI CC 120 semantics.
    void allSoundOff(int channel = -1);

    // Arpeggiator (global, operates on current active part)
    Arpeggiator& arpeggiator() { return arpeggiator_; }
    void setArpEnabled(bool e) { arpeggiator_.setEnabled(e); }
    void setArpTempo(float bpm) { arpeggiator_.setTempo(bpm); }
    void setArpPattern(int p) { arpeggiator_.setPattern(p); }
    void setArpOctaveRange(int o) { arpeggiator_.setOctaveRange(o); }
    void setArpGate(float g) { arpeggiator_.setGate(g); }
    void setArpResolution(int r) { arpeggiator_.setResolution(r); }
    void setArpSwing(float s) { arpeggiator_.setSwing(s); }
    void setArpHold(bool h) { arpeggiator_.setHold(h); }
    int getArpCurrentStep() const { return arpeggiator_.currentStep(); }
    int getArpTotalSteps() const { return arpeggiator_.totalSteps(); }

    // Voice allocation
    VoiceAllocator& allocator() { return allocator_; }
    void setVoicePriorityMode(int m) { allocator_.setPriorityMode(static_cast<VoicePriorityMode>(m)); }

    // Thread-safe parameter queue
    ParamQueue& paramQueue() { return paramQueue_; }

    // ── Part access ──
    SynthPart& part(int index) { return parts_[index]; }
    const SynthPart& part(int index) const { return parts_[index]; }
    void setPartMidiChannel(int partIndex, int channel);  // -1=off, 0-15=ch1-16
    void setPartVolume(int partIndex, float vol);
    void setPartPan(int partIndex, float pan);
    void setPartMute(int partIndex, bool mute);
    void setPartSolo(int partIndex, bool solo);

    // ── Legacy direct setters (operate on part 0 for backward compat) ──
    void setOsc1Waveform(int w) { parts_[0].osc1.setWaveform(w); }
    void setOsc1Octave(int oct) { parts_[0].osc1.setOctave(oct); }
    void setOsc1Detune(float cents) { parts_[0].osc1.setDetune(cents); }
    void setOsc1PulseWidth(float pw) { parts_[0].osc1.setPulseWidth(pw); }
    void setOsc1Volume(float vol) { parts_[0].osc1.setVolume(vol); }
    void setOsc1NoiseType(int nt) { parts_[0].osc1.setNoiseType(nt); }
    void setOsc1SubOscMode(int m) { parts_[0].osc1.setSubOscMode(m); }
    void setOsc1SubOscVolume(float v) { parts_[0].osc1.setSubOscVolume(v); }
    void setOsc1FmEnabled(bool e) { parts_[0].osc1.setFmEnabled(e); }
    void setOsc1FmAmount(float a) { parts_[0].osc1.setFmAmount(a); }

    void setOsc2Waveform(int w) { parts_[0].osc2.setWaveform(w); }
    void setOsc2Octave(int oct) { parts_[0].osc2.setOctave(oct); }
    void setOsc2Detune(float cents) { parts_[0].osc2.setDetune(cents); }
    void setOsc2PulseWidth(float pw) { parts_[0].osc2.setPulseWidth(pw); }
    void setOsc2Volume(float vol) { parts_[0].osc2.setVolume(vol); }
    void setOsc2NoiseType(int nt) { parts_[0].osc2.setNoiseType(nt); }
    void setOsc2SubOscMode(int m) { parts_[0].osc2.setSubOscMode(m); }
    void setOsc2SubOscVolume(float v) { parts_[0].osc2.setSubOscVolume(v); }
    void setOsc2FmEnabled(bool e) { parts_[0].osc2.setFmEnabled(e); }
    void setOsc2FmAmount(float a) { parts_[0].osc2.setFmAmount(a); }
    void setOscMix(float mix) { parts_[0].oscMix = mix; }

    void setOsc1UnisonVoiceCount(int c) { parts_[0].osc1.setUnisonVoiceCount(c); }
    void setOsc1UnisonDetuneSpread(float s) { parts_[0].osc1.setUnisonDetuneSpread(s); }
    void setOsc1UnisonStereoSpread(float s) { parts_[0].osc1.setUnisonStereoSpread(s); }
    void setOsc1UnisonMix(float m) { parts_[0].osc1.setUnisonMix(m); }

    void setOsc2UnisonVoiceCount(int c) { parts_[0].osc2.setUnisonVoiceCount(c); }
    void setOsc2UnisonDetuneSpread(float s) { parts_[0].osc2.setUnisonDetuneSpread(s); }
    void setOsc2UnisonStereoSpread(float s) { parts_[0].osc2.setUnisonStereoSpread(s); }
    void setOsc2UnisonMix(float m) { parts_[0].osc2.setUnisonMix(m); }

    void setFilterType(int t) { parts_[0].filter.setType(t); }
    void setFilterCutoff(float hz) { parts_[0].filter.setCutoff(hz); }
    void setFilterResonance(float q) { parts_[0].filter.setResonance(q); }
    void setFilterEnvAmount(float a) { parts_[0].filter.setEnvAmount(a); }
    void setFilterKeyTracking(float kt) { parts_[0].filter.setKeyTracking(kt); }
    void setFilterDrive(float d) { parts_[0].filter.setDrive(d); }

    void setAmpAttack(float ms) { parts_[0].ampAttack = ms; }
    void setAmpDecay(float ms) { parts_[0].ampDecay = ms; }
    void setAmpSustain(float level) { parts_[0].ampSustain = level; }
    void setAmpRelease(float ms) { parts_[0].ampRelease = ms; }
    void setAmpDelay(float ms) { parts_[0].ampDelay = ms; }
    void setAmpHold(float ms) { parts_[0].ampHold = ms; }
    void setAmpAttackCurve(int c) { parts_[0].ampAttackCurve = c; }
    void setAmpDecayCurve(int c) { parts_[0].ampDecayCurve = c; }
    void setAmpReleaseCurve(int c) { parts_[0].ampReleaseCurve = c; }

    void setFilterAttack(float ms) { parts_[0].filterAttack = ms; }
    void setFilterDecay(float ms) { parts_[0].filterDecay = ms; }
    void setFilterSustain(float level) { parts_[0].filterSustain = level; }
    void setFilterRelease(float ms) { parts_[0].filterRelease = ms; }
    void setFilterDelay(float ms) { parts_[0].filterDelay = ms; }
    void setFilterHold(float ms) { parts_[0].filterHold = ms; }
    void setFilterAttackCurve(int c) { parts_[0].filterAttackCurve = c; }
    void setFilterDecayCurve(int c) { parts_[0].filterDecayCurve = c; }
    void setFilterReleaseCurve(int c) { parts_[0].filterReleaseCurve = c; }

    void setPitchEnvAttack(float ms) { parts_[0].pitchEnvAttack = ms; }
    void setPitchEnvDecay(float ms) { parts_[0].pitchEnvDecay = ms; }
    void setPitchEnvSustain(float level) { parts_[0].pitchEnvSustain = level; }
    void setPitchEnvRelease(float ms) { parts_[0].pitchEnvRelease = ms; }
    void setPitchEnvAmount(float a) { parts_[0].pitchEnvAmount = a; }

    void setLfo1Waveform(int w) { parts_[0].lfo1.setWaveform(w); }
    void setLfo1Rate(float hz) { parts_[0].lfo1.setRate(hz); }
    void setLfo1Depth(float d) { parts_[0].lfo1.setDepth(d); }
    void setLfo1Target(int t) { parts_[0].lfo1.setTarget(t); }
    void setLfo1FadeIn(float s) { parts_[0].lfo1.setFadeIn(s); }
    void setLfo1TempoSync(bool e) { parts_[0].lfo1.setTempoSync(e); }
    void setLfo1TempoDivision(int d) { parts_[0].lfo1.setTempoNoteDivision(d); }

    void setLfo2Waveform(int w) { parts_[0].lfo2.setWaveform(w); }
    void setLfo2Rate(float hz) { parts_[0].lfo2.setRate(hz); }
    void setLfo2Depth(float d) { parts_[0].lfo2.setDepth(d); }
    void setLfo2Target(int t) { parts_[0].lfo2.setTarget(t); }
    void setLfo2FadeIn(float s) { parts_[0].lfo2.setFadeIn(s); }
    void setLfo2TempoSync(bool e) { parts_[0].lfo2.setTempoSync(e); }
    void setLfo2TempoDivision(int d) { parts_[0].lfo2.setTempoNoteDivision(d); }

    // FX Engine
    FxEngine& fxEngine() { return fxEngine_; }
    const FxEngine& fxEngine() const { return fxEngine_; }

    // Legacy FX setters
    void setChorusEnabled(bool e);
    void setChorusRate(float hz);
    void setChorusDepth(float d);
    void setChorusMix(float m);

    void setDelayEnabled(bool e);
    void setDelayTime(float ms);
    void setDelayFeedback(float fb);
    void setDelayMix(float m);

    void setReverbEnabled(bool e);
    void setReverbSize(float s);
    void setReverbDamping(float d);
    void setReverbMix(float m);

    void setPhaserEnabled(bool e);
    void setPhaserRate(float hz);
    void setPhaserDepth(float d);
    void setPhaserFeedback(float fb);
    void setPhaserMix(float m);

    void setFlangerEnabled(bool e);
    void setFlangerRate(float hz);
    void setFlangerDepth(float d);
    void setFlangerFeedback(float fb);
    void setFlangerMix(float m);

    void setCompressorEnabled(bool e);
    void setCompressorThreshold(float t);
    void setCompressorRatio(float r);
    void setCompressorAttack(float a);
    void setCompressorRelease(float r);
    void setCompressorMakeupGain(float g);

    void setDriveEnabled(bool e);
    void setDriveAmount(float a);
    void setDriveType(int t);

    // Master
    void setMasterVolume(float vol) { masterVolume_ = vol; }
    int getActiveVoiceCount() const { return allocator_.activeVoiceCount(); }

    // ── MIDI performance controls (part 0 for now) ──
    void setPitchBend(int wheelValue) {
        // wheelValue is 0..16383, center = 8192
        float normalized = (wheelValue - 8192) / 8192.0f;
        parts_[0].pitchBend = normalized;
        if (samplePlayer_)
            samplePlayer_->setPitchBend(normalized * 2.0f); // +/- 2 semitones
    }
    void setModWheel(float value) {
        parts_[0].modWheel = value;
    }
    void setAftertouch(float value) {
        parts_[0].aftertouch = value;
    }
    void setPolyAftertouch(int /*note*/, float value) {
        parts_[0].aftertouch = value; // mono for now
    }

    // CPU profiling
    float getCpuLoad() const { return cpuLoad_; }
    void resetCpuLoad() { cpuLoad_ = 0.0f; cpuLoadAlpha_ = 0.0f; }

    // Preset
    int loadPreset(const char* path);
    int savePreset(const char* path) const;

    // Info
    const char* getName() const { return "OpenAmp Synth Engine"; }
    const char* getVersion() const { return "2.2.0"; }

    // Recording
    void startRecording(const char* path) { recorder_.startRecording(path); }
    void stopRecording() { recorder_.stop(); }
    bool isRecording() const { return recorder_.state() == TransportState::RECORDING; }
    double recordedSeconds() const { return recorder_.recordedSeconds(); }

    // MPE controller
    MpeController& mpeController() { return mpeController_; }
    const MpeController& mpeController() const { return mpeController_; }

private:
    double sampleRate_;
    uint32_t blockSize_;

    ParamQueue paramQueue_;

    // 16-part multitimbral configuration
    std::array<SynthPart, MAX_PARTS> parts_;
    int activePart_ = 0;  // Part selected for editing/arp

    // Global voice pool (dynamically allocated across parts)
    VoiceAllocator allocator_;

    // MPE controller
    MpeController mpeController_;

    // Arpeggiator
    Arpeggiator arpeggiator_;

    // Drum Kit
    DrumKit drumKit_;

    // Rhythm Pattern Player — preset drum patterns
    RhythmPatternPlayer rhythmPlayer_;

    // Recording engine
    Recorder recorder_;

public:
    RhythmPatternPlayer& rhythmPlayer() { return rhythmPlayer_; }
    const RhythmPatternPlayer& rhythmPlayer() const { return rhythmPlayer_; }

private:
    DrumKit& drumKit() { return drumKit_; }
    const DrumKit& drumKit() const { return drumKit_; }
    void drumNoteOn(int midiNote, float velocity) { drumKit_.noteOn(midiNote, velocity); }
    void drumNoteOff(int midiNote) { drumKit_.noteOff(midiNote); }
    void setDrumKitPreset(int index) { drumKit_.setKitPreset(index); }

public:
    void setDrumKitLevel(float level) { drumKit_.setLevel(level); }

private:
    FxEngine fxEngine_;

    // Global sympathetic resonance — shared across all voices
    SympatheticResonator sympatheticResonator_;

public:
    // Sample player accessor
    SamplePlayer* getSamplePlayer() const { return samplePlayer_.get(); }
    void setSamplePlayer(std::unique_ptr<SamplePlayer> sp) { samplePlayer_ = std::move(sp); }

    // Sample rate accessor
    double getSampleRate() const { return sampleRate_; }

private:
    // Sample playback / ROMpler layer
    std::unique_ptr<SamplePlayer> samplePlayer_;

    float masterVolume_ = 0.8f;

    // Solo tracking
    bool anySolo_ = false;
    void updateSoloState();

    // Legacy FX accessor
public:
    LegacyFxProcessor* getLegacyFx() const;
private:
    void initLegacyFxSlot();

public:
    FxProcessor* createFxProcessor(int fxTypeId);

private:
    mutable float cpuLoad_ = 0.0f;
    mutable float cpuLoadAlpha_ = 0.0f;

    template<typename T>
    static T clamp(T v, T lo, T hi) { return v < lo ? lo : (v > hi ? hi : v); }

    void drainQueue();
    void applyParam(const ParamQueue::Entry& e);

    // MIDI channel -> part index lookup
    int channelToPart(int channel) const;

    // Performance-mode internals
    void doNoteOn(int midiNote, float velocity, int partIdx, int mpeChannel);
    void doNoteOff(int midiNote, int partIdx, int mpeChannel);
    void configureSplitPart(); // Karplus bass patch on part 1

    bool perfSplitEnabled_ = false;
    int perfSplitPoint_ = 60;   // C4 — first note of the upper zone
    bool perfLayerEnabled_ = false;
    int perfTranspose_ = 0;
    // Routing record per original note so noteOff releases the exact
    // transposed note/parts even if settings changed while held.
    struct RoutedNote { int note = -1; uint32_t partMask = 0; };
    RoutedNote routedNotes_[128];
};

} // namespace opensynth
