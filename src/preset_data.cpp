#include "preset_data.h"
#include "synth_engine_wrapper.h"
#include "synth_engine.h"
#include <juce_core/juce_core.h>

namespace opensynth {

// Forward declaration
juce::File getSampleManifestPath(const char* id);

void applyPresetToEngine(const PresetData& p, SynthEngineWrapper& e)
{
    // Sanitize extreme values that cause harsh/static sounds
    auto clamp = [](float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); };

    // Osc 1
    e.setOsc1Waveform(p.osc1Waveform);
    e.setOsc1Octave(p.osc1Octave);
    e.setOsc1Detune(p.osc1Detune);
    e.setOsc1Volume(clamp(p.osc1Volume, 0.0f, 1.0f));

    // Osc 2
    e.setOsc2Waveform(p.osc2Waveform);
    e.setOsc2Octave(p.osc2Octave);
    e.setOsc2Detune(p.osc2Detune);
    e.setOsc2Volume(clamp(p.osc2Volume, 0.0f, 1.0f));
    e.setOscMix(clamp(p.oscMix, 0.0f, 1.0f));

    // Filter — clamp resonance to prevent ear-bleed
    e.setFilterCutoff(clamp(p.filterCutoff, 20.0f, 20000.0f));
    e.setFilterResonance(clamp(p.filterResonance, 0.0f, 0.75f)); // was up to 0.95
    e.setFilterEnvAmount(clamp(p.filterEnvAmount, -1.0f, 1.0f));
    e.setFilterDrive(clamp(p.filterDrive, 0.0f, 0.8f));

    // Amp envelope — clamp attack to prevent 3-second silence
    e.setAmpAttack(clamp(p.ampAttack, 0.1f, 2000.0f)); // was up to 5000ms
    e.setAmpDecay(clamp(p.ampDecay, 0.1f, 3000.0f));
    e.setAmpSustain(clamp(p.ampSustain, 0.0f, 1.0f));
    e.setAmpRelease(clamp(p.ampRelease, 0.1f, 5000.0f));

    // Filter envelope
    e.setFilterAttack(clamp(p.filterAttack, 0.1f, 2000.0f));
    e.setFilterDecay(clamp(p.filterDecay, 0.1f, 3000.0f));
    e.setFilterSustain(clamp(p.filterSustain, 0.0f, 1.0f));
    e.setFilterRelease(clamp(p.filterRelease, 0.1f, 5000.0f));

    // LFO 1
    e.setLfo1Rate(clamp(p.lfo1Rate, 0.1f, 20.0f));
    e.setLfo1Depth(clamp(p.lfo1Depth, 0.0f, 1.0f));

    // Master — ensure we don't blow ears out
    e.setMasterVolume(clamp(p.masterVolume, 0.0f, 1.0f));

    // FX slots
    for (int slot = 0; slot < 3; ++slot)
    {
        e.setFxType(slot + 1, p.fxSlotType[slot]);
        e.setFxEnabled(slot + 1, p.fxSlotEnabled[slot]);
        for (int param = 0; param < 4; ++param)
            e.setFxParam(slot + 1, param, clamp(p.fxSlotParam[slot][param], 0.0f, 1.0f));
    }

    // Instrument realism
    e.setRealismBodyType(p.realismBodyType);
    e.setRealismBodyMix(clamp(p.realismBodyMix, 0.0f, 1.0f));
    e.setRealismClickMix(clamp(p.realismClickMix, 0.0f, 1.0f));
    e.setRealismSympatheticMix(clamp(p.realismSympatheticMix, 0.0f, 1.0f));
    e.setRealismAttackCurve(p.realismAttackCurve);
    e.setRealismBrightnessSens(clamp(p.realismBrightnessSens, 0.0f, 1.0f));

    // Arpeggiator
    e.setArpEnabled(p.arpEnabled);
    e.setArpPattern(p.arpPattern);
    e.setArpTempo(clamp(p.arpTempo, 20.0f, 300.0f));
    e.setArpGate(clamp(p.arpGate, 0.0f, 1.0f));
    e.setArpSwing(clamp(p.arpSwing, 0.0f, 1.0f));
    e.setArpOctaveRange(p.arpOctave > 4 ? 4 : (p.arpOctave < 1 ? 1 : p.arpOctave));

    // Sample manifest loading
    if (p.sampleManifestId[0] != '\0' && e.getEngine()) {
        auto* player = e.getEngine()->getSamplePlayer();
        if (player) {
            juce::File manifestFile = getSampleManifestPath(p.sampleManifestId);
            if (manifestFile.existsAsFile()) {
                player->clear();
                player->loadMultiSample(manifestFile.getFullPathName().toStdString());
                player->setMixLevel(clamp(p.sampleMix, 0.0f, 1.0f));
            }
        }
    }
}

// Helper to resolve manifest path relative to executable, user data dir, or dev tree
juce::File getSampleManifestPath(const char* id)
{
    const auto fileName = juce::String(id) + ".json";

    // 1. Next to the executable (standalone bundles ship samples/ beside the binary)
    juce::File exeDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();
    juce::File manifest = exeDir.getChildFile("samples/manifests").getChildFile(fileName);
    if (manifest.existsAsFile()) return manifest;

    // 2. One level up (macOS .app bundle structure)
    manifest = exeDir.getParentDirectory().getChildFile("samples/manifests").getChildFile(fileName);
    if (manifest.existsAsFile()) return manifest;

    // 2b. Beside the plugin module itself. In a DAW the "executable" is the
    //     host, but currentApplicationFile is the VST3/CLAP/AU file, and the
    //     build/installers place samples/ inside or next to it.
    {
        const juce::File plugin = juce::File::getSpecialLocation(juce::File::currentApplicationFile);
        const juce::File candidates[] = {
            plugin.getParentDirectory(),                               // CLAP: <dir>/samples, VST3 .so/.dll dir
            plugin.getChildFile("Contents/MacOS"),                     // macOS bundles
            plugin.getChildFile("Contents/Resources"),
            plugin.getChildFile("Contents/x86_64-linux"),              // Linux VST3 bundle
            plugin.getChildFile("Contents/x86_64-win"),                // Windows VST3 bundle
            plugin.getParentDirectory().getParentDirectory(),
        };
        for (const auto& dir : candidates) {
            manifest = dir.getChildFile("samples/manifests").getChildFile(fileName);
            if (manifest.existsAsFile()) return manifest;
        }
    }

    // 2c. System-wide shared library (installers may place samples here):
    //     Windows: C:\ProgramData\OpenSynth\samples, macOS: /Library/OpenSynth/samples
    manifest = juce::File::getSpecialLocation(juce::File::commonApplicationDataDirectory)
                   .getChildFile("OpenSynth/samples/manifests").getChildFile(fileName);
    if (manifest.existsAsFile()) return manifest;

    // 3. User data directory — the canonical install location for plugin
    //    (VST3/CLAP) use, where the "executable" is the DAW, not Open Synth.
    //    Linux: ~/.config/OpenSynth/samples
    //    Windows: %APPDATA%\OpenSynth\samples
    manifest = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("OpenSynth/samples/manifests").getChildFile(fileName);
    if (manifest.existsAsFile()) return manifest;

    // 4. Development tree fallback (running from a repo checkout)
    manifest = juce::File::getCurrentWorkingDirectory().getChildFile("samples/manifests").getChildFile(fileName);
    if (manifest.existsAsFile()) return manifest;

    return exeDir.getChildFile("samples/manifests").getChildFile(fileName);
}

void applyPresetToAPVTS(const PresetData& p, juce::AudioProcessorValueTreeState& apvts)
{
    auto setFloat = [&](const juce::String& id, float v) {
        if (auto* param = apvts.getParameter(id))
            param->setValueNotifyingHost(param->convertTo0to1(v));
    };
    auto setInt = [&](const juce::String& id, int v) {
        if (auto* param = apvts.getParameter(id))
            param->setValueNotifyingHost(param->convertTo0to1((float)v));
    };
    auto setBool = [&](const juce::String& id, bool b) {
        if (auto* param = apvts.getParameter(id))
            param->setValueNotifyingHost(b ? 1.0f : 0.0f);
    };

    setInt("osc1Waveform", p.osc1Waveform);
    setInt("osc1Octave", p.osc1Octave);
    setFloat("osc1Detune", p.osc1Detune);
    setFloat("osc1Volume", p.osc1Volume);

    setInt("osc2Waveform", p.osc2Waveform);
    setInt("osc2Octave", p.osc2Octave);
    setFloat("osc2Detune", p.osc2Detune);
    setFloat("osc2Volume", p.osc2Volume);
    setFloat("oscMix", p.oscMix);

    setFloat("filterCutoff", p.filterCutoff);
    setFloat("filterResonance", p.filterResonance);
    setFloat("filterEnvAmt", p.filterEnvAmount);
    setFloat("filterDrive", p.filterDrive);

    setFloat("ampAttack", p.ampAttack);
    setFloat("ampDecay", p.ampDecay);
    setFloat("ampSustain", p.ampSustain);
    setFloat("ampRelease", p.ampRelease);

    setFloat("filterAttack", p.filterAttack);
    setFloat("filterDecay", p.filterDecay);
    setFloat("filterSustain", p.filterSustain);
    setFloat("filterRelease", p.filterRelease);

    setFloat("lfo1Rate", p.lfo1Rate);
    setFloat("lfo1Depth", p.lfo1Depth);

    setFloat("masterVolume", p.masterVolume);

    setFloat("sampleMix", p.sampleMix);

    setBool("fx1Enabled", p.fxSlotEnabled[0]);
    setInt("fx1Type", p.fxSlotType[0]);
    setFloat("fx1Param0", p.fxSlotParam[0][0]);
    setFloat("fx1Param1", p.fxSlotParam[0][1]);
    setFloat("fx1Param2", p.fxSlotParam[0][2]);
    setFloat("fx1Param3", p.fxSlotParam[0][3]);

    setBool("fx2Enabled", p.fxSlotEnabled[1]);
    setInt("fx2Type", p.fxSlotType[1]);
    setFloat("fx2Param0", p.fxSlotParam[1][0]);
    setFloat("fx2Param1", p.fxSlotParam[1][1]);
    setFloat("fx2Param2", p.fxSlotParam[1][2]);
    setFloat("fx2Param3", p.fxSlotParam[1][3]);

    setBool("fx3Enabled", p.fxSlotEnabled[2]);
    setInt("fx3Type", p.fxSlotType[2]);
    setFloat("fx3Param0", p.fxSlotParam[2][0]);
    setFloat("fx3Param1", p.fxSlotParam[2][1]);
    setFloat("fx3Param2", p.fxSlotParam[2][2]);
    setFloat("fx3Param3", p.fxSlotParam[2][3]);

    setBool("arpEnabled", p.arpEnabled);
    setInt("arpPattern", p.arpPattern);
    setFloat("arpTempo", p.arpTempo);
    setFloat("arpGate", p.arpGate);
    setFloat("arpSwing", p.arpSwing);
    setInt("arpOctave", p.arpOctave);
}

} // namespace opensynth
