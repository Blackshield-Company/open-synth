#pragma once
#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace opensynth {

struct UserPreset {
    juce::String name;
    juce::String category;
    juce::StringArray tags;
    juce::var parameterValues; // JSON object with paramID -> value
    juce::Time creationTime;

    juce::var toJSON() const;
    static std::optional<UserPreset> fromJSON(const juce::var& json);
};

class UserPresetManager {
public:
    UserPresetManager();
    // Use a custom presets directory (tests / portable installs)
    explicit UserPresetManager(const juce::File& presetsDirectory);

    // Save current APVTS state as a user preset
    bool savePreset(const juce::String& name, const juce::String& category,
                    const juce::StringArray& tags,
                    juce::AudioProcessorValueTreeState& apvts);

    // Load a user preset into APVTS
    bool loadPreset(const UserPreset& preset, juce::AudioProcessorValueTreeState& apvts);

    // Delete a user preset by name
    bool deletePreset(const juce::String& name);

    // Get all user presets
    std::vector<UserPreset> getUserPresets() const;

    // Merge presets from a JSON file (array of presets, a single preset
    // object, or {"presets": [...]}) into user_presets.json. Presets with
    // the same name are replaced. Returns the number imported, -1 on error.
    int importPresetsFromFile(const juce::File& file);

    // Parameter-list level helpers (APVTS overloads forward to these).
    // Values are keyed by stable parameter ID. Loading also accepts legacy
    // presets keyed by display name.
    static juce::var captureParameters(const juce::Array<juce::AudioProcessorParameter*>& params);
    static int applyParameters(const juce::var& values,
                               const juce::Array<juce::AudioProcessorParameter*>& params);
    static juce::String getParameterKey(const juce::AudioProcessorParameter& param);

    // Get user presets directory
    static juce::File getPresetsDirectory();

private:
    juce::File presetsDirectory_;
    juce::File getPresetsFile() const;
    void writePresetsToDisk(const std::vector<UserPreset>& presets) const;
};

} // namespace opensynth
