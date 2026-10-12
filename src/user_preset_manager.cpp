#include "user_preset_manager.h"

namespace opensynth {

juce::var UserPreset::toJSON() const
{
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("name", name);
    obj->setProperty("category", category);

    juce::Array<juce::var> tagArray;
    for (const auto& tag : tags)
        tagArray.add(tag);
    obj->setProperty("tags", tagArray);

    obj->setProperty("parameters", parameterValues);
    obj->setProperty("creationTime", creationTime.toMilliseconds());

    return juce::var(obj);
}

std::optional<UserPreset> UserPreset::fromJSON(const juce::var& json)
{
    if (!json.isObject())
        return std::nullopt;

    UserPreset preset;
    preset.name = json.getProperty("name", "").toString();
    preset.category = json.getProperty("category", "").toString();

    if (auto* tagArray = json.getProperty("tags", {}).getArray())
    {
        for (const auto& tag : *tagArray)
            preset.tags.add(tag.toString());
    }

    preset.parameterValues = json.getProperty("parameters", {});
    preset.creationTime = juce::Time(json.getProperty("creationTime", juce::int64(0)));

    return preset;
}

UserPresetManager::UserPresetManager()
    : UserPresetManager(getPresetsDirectory())
{
}

UserPresetManager::UserPresetManager(const juce::File& presetsDirectory)
    : presetsDirectory_(presetsDirectory)
{
    presetsDirectory_.createDirectory();
}

juce::String UserPresetManager::getParameterKey(const juce::AudioProcessorParameter& param)
{
    if (auto* withId = dynamic_cast<const juce::AudioProcessorParameterWithID*>(&param))
        return withId->getParameterID();
    return param.getName(50);
}

juce::var UserPresetManager::captureParameters(const juce::Array<juce::AudioProcessorParameter*>& params)
{
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    for (auto* param : params)
        if (param != nullptr)
            obj->setProperty(getParameterKey(*param), param->getValue());
    return juce::var(obj.get());
}

int UserPresetManager::applyParameters(const juce::var& values,
                                       const juce::Array<juce::AudioProcessorParameter*>& params)
{
    auto* obj = values.getDynamicObject();
    if (obj == nullptr)
        return -1;

    // A preset is ID-keyed if any key matches a parameter ID. Legacy presets
    // (v2.1.0 and earlier) were keyed by display name; those still load, with
    // the old (ambiguous) behaviour for params sharing a display name.
    bool idKeyed = false;
    for (auto* param : params)
        if (param != nullptr && obj->hasProperty(getParameterKey(*param)))
            { idKeyed = true; break; }

    int applied = 0;
    for (auto* param : params)
    {
        if (param == nullptr)
            continue;
        juce::String key = idKeyed ? getParameterKey(*param) : param->getName(50);
        if (obj->hasProperty(key))
        {
            float v = juce::jlimit(0.0f, 1.0f, static_cast<float>(obj->getProperty(key)));
            param->setValueNotifyingHost(v);
            ++applied;
        }
    }
    return applied;
}

juce::File UserPresetManager::getPresetsDirectory()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("open-synth")
        .getChildFile("presets");
}

juce::File UserPresetManager::getPresetsFile() const
{
    return presetsDirectory_.getChildFile("user_presets.json");
}

std::vector<UserPreset> UserPresetManager::getUserPresets() const
{
    std::vector<UserPreset> presets;

    auto file = getPresetsFile();
    if (!file.existsAsFile())
        return presets;

    auto json = juce::JSON::parse(file);
    if (auto* array = json.getArray())
    {
        for (const auto& item : *array)
        {
            if (auto preset = UserPreset::fromJSON(item))
                presets.push_back(*preset);
        }
    }

    return presets;
}

void UserPresetManager::writePresetsToDisk(const std::vector<UserPreset>& presets) const
{
    juce::Array<juce::var> array;
    for (const auto& p : presets)
        array.add(p.toJSON());

    auto file = getPresetsFile();
    auto jsonString = juce::JSON::toString(juce::var(array), true);
    file.replaceWithText(jsonString);
}

bool UserPresetManager::savePreset(const juce::String& name, const juce::String& category,
                                   const juce::StringArray& tags,
                                   juce::AudioProcessorValueTreeState& apvts)
{
    if (name.trim().isEmpty())
        return false;

    UserPreset preset;
    preset.name = name.trim();
    preset.category = category.isEmpty() ? "Custom" : category;
    preset.tags = tags;
    preset.creationTime = juce::Time::getCurrentTime();

    // Capture all parameter values, keyed by stable parameter ID
    preset.parameterValues = captureParameters(apvts.processor.getParameters());

    auto presets = getUserPresets();

    // Replace existing preset with same name
    bool replaced = false;
    for (auto& existing : presets)
    {
        if (existing.name == preset.name)
        {
            existing = preset;
            replaced = true;
            break;
        }
    }

    if (!replaced)
        presets.push_back(preset);

    writePresetsToDisk(presets);
    return true;
}

bool UserPresetManager::loadPreset(const UserPreset& preset, juce::AudioProcessorValueTreeState& apvts)
{
    if (!preset.parameterValues.isObject())
        return false;

    return applyParameters(preset.parameterValues, apvts.processor.getParameters()) >= 0;
}

int UserPresetManager::importPresetsFromFile(const juce::File& file)
{
    if (!file.existsAsFile())
        return -1;

    juce::var json;
    if (juce::JSON::parse(file.loadFileAsString(), json).failed())
        return -1;

    if (json.isObject() && json.getProperty("presets", {}).isArray())
        json = json.getProperty("presets", {});

    std::vector<UserPreset> incoming;
    if (auto* array = json.getArray())
    {
        for (const auto& item : *array)
            if (auto p = UserPreset::fromJSON(item); p && p->name.trim().isNotEmpty() && p->parameterValues.isObject())
                incoming.push_back(*p);
    }
    else if (auto p = UserPreset::fromJSON(json); p && p->name.trim().isNotEmpty() && p->parameterValues.isObject())
    {
        incoming.push_back(*p);
    }

    if (incoming.empty())
        return json.isArray() ? 0 : -1;

    auto presets = getUserPresets();
    for (auto& p : incoming)
    {
        p.name = p.name.trim();
        if (p.category.isEmpty())
            p.category = "Custom";
        auto it = std::find_if(presets.begin(), presets.end(),
                               [&p](const UserPreset& e) { return e.name == p.name; });
        if (it != presets.end())
            *it = p;
        else
            presets.push_back(p);
    }

    writePresetsToDisk(presets);
    return (int)incoming.size();
}

bool UserPresetManager::deletePreset(const juce::String& name)
{
    auto presets = getUserPresets();
    auto it = std::remove_if(presets.begin(), presets.end(),
                             [&name](const UserPreset& p) { return p.name == name; });

    if (it == presets.end())
        return false;

    presets.erase(it, presets.end());
    writePresetsToDisk(presets);
    return true;
}

} // namespace opensynth
