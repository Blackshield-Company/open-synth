// User preset tests: values are keyed by stable parameter ID (params that
// share a display name stay independent), legacy display-name presets still
// load, and importing a JSON file merges into user_presets.json.
#include <juce_audio_processors/juce_audio_processors.h>
#include "plugin_processor.h"
#include "user_preset_manager.h"
#include <map>

using namespace opensynth;

static int g_failures = 0;

#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            ++g_failures;                                               \
        }                                                               \
    } while (0)


int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    auto dir = juce::File::createTempFile("open-synth-preset-test");
    dir.createDirectory();

    OpenSynthProcessor proc;
    auto& apvts = proc.getParameters();
    const auto& params = proc.getParameters().processor.getParameters();

    // ── Every parameter has a unique ID; display names collide ───────
    std::map<juce::String, int> nameCount;
    juce::StringArray ids;
    for (auto* p : params)
    {
        ids.add(UserPresetManager::getParameterKey(*p));
        ++nameCount[p->getName(50)];
    }
    juce::StringArray uniqueIds(ids);
    uniqueIds.removeDuplicates(false);
    CHECK(uniqueIds.size() == ids.size());
    std::printf("%d params, %d display names\n", ids.size(), (int)nameCount.size());

    // Pick a group of parameters that share a display name
    juce::String sharedName;
    for (auto& [n, c] : nameCount)
        if (c > 1) { sharedName = n; break; }
    CHECK(sharedName.isNotEmpty());
    juce::Array<juce::AudioProcessorParameter*> group;
    for (auto* p : params)
        if (p->getName(50) == sharedName)
            group.add(p);

    // ── Save writes IDs; round trip keeps same-named params independent ─
    {
        UserPresetManager mgr(dir);
        for (int i = 0; i < group.size(); ++i)
            group[i]->setValueNotifyingHost(i % 2 == 0 ? 1.0f : 0.0f);
        CHECK(mgr.savePreset("Independent", "Custom", {}, apvts));

        auto presets = mgr.getUserPresets();
        CHECK(presets.size() == 1);
        auto* obj = presets[0].parameterValues.getDynamicObject();
        CHECK(obj != nullptr);
        if (obj != nullptr)
        {
            CHECK(obj->getProperties().size() == params.size());
            for (const auto& id : ids)
                CHECK(obj->hasProperty(id));
        }

        // Scramble, then reload
        for (auto* p : group)
            p->setValueNotifyingHost(0.5f);
        CHECK(mgr.loadPreset(presets[0], apvts));
        for (int i = 0; i < group.size(); ++i)
            CHECK(std::abs(group[i]->getValue() - (i % 2 == 0 ? 1.0f : 0.0f)) < 1e-4f);
    }

    // ── Legacy display-name presets still load ───────────────────────
    {
        auto* first = params[0];
        juce::DynamicObject::Ptr legacy = new juce::DynamicObject();
        legacy->setProperty(first->getName(50), 0.25f);
        first->setValueNotifyingHost(0.9f);
        CHECK(UserPresetManager::applyParameters(juce::var(legacy.get()), params) >= 1);
        float expect = 0.25f;
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(first))
            expect = rp->convertTo0to1(rp->convertFrom0to1(0.25f));
        CHECK(std::abs(first->getValue() - expect) < 1e-3f);
    }

    // ── Import merges a chosen JSON file ─────────────────────────────
    {
        UserPresetManager mgr(dir);
        auto paramId = ids[0];
        juce::String json = "[{\"name\":\"Independent\",\"category\":\"Pads\",\"parameters\":{\""
                          + paramId + "\":0.0}},"
                            "{\"name\":\"Imported Two\",\"parameters\":{\"" + paramId + "\":1.0}},"
                            "{\"name\":\"\",\"parameters\":{}}]";
        auto file = dir.getChildFile("import.json");
        file.replaceWithText(json);
        CHECK(mgr.importPresetsFromFile(file) == 2);
        auto presets = mgr.getUserPresets();
        CHECK(presets.size() == 2);
        CHECK(presets[0].name == "Independent" && presets[0].category == "Pads");
        CHECK(presets[1].name == "Imported Two" && presets[1].category == "Custom");

        // {"presets": [...]} wrapper and a single object are accepted too
        file.replaceWithText("{\"presets\":[{\"name\":\"Three\",\"parameters\":{\"" + paramId + "\":0.5}}]}");
        CHECK(mgr.importPresetsFromFile(file) == 1);
        file.replaceWithText("{\"name\":\"Four\",\"parameters\":{\"" + paramId + "\":0.5}}");
        CHECK(mgr.importPresetsFromFile(file) == 1);
        CHECK(mgr.getUserPresets().size() == 4);

        // Garbage / missing files fail without touching the store
        file.replaceWithText("not json {");
        CHECK(mgr.importPresetsFromFile(file) == -1);
        CHECK(mgr.importPresetsFromFile(dir.getChildFile("missing.json")) == -1);
        CHECK(mgr.getUserPresets().size() == 4);
    }

    dir.deleteRecursively();
    if (g_failures == 0) std::printf("user_preset_test: all checks passed\n");
    return g_failures == 0 ? 0 : 1;
}
