#include "PresetManager.h"

namespace mfpr
{
PresetManager::PresetManager(AssetLibrary& l, juce::AudioProcessorValueTreeState& vts) : library(l), apvts(vts) {}

juce::String PresetManager::macroParamId(int macroIndex)
{
    return juce::String::formatted("macro%02d", macroIndex);
}

juce::Array<int> PresetManager::getCurrentMacros() const
{
    juce::Array<int> macros;
    macros.ensureStorageAllocated(13);
    for (int i = 1; i <= 13; ++i)
    {
        if (auto* p = dynamic_cast<juce::AudioParameterInt*>(apvts.getParameter(macroParamId(i))))
            macros.add(p->get());
        else if (auto* pf = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(macroParamId(i))))
            macros.add(juce::roundToInt(pf->get() * 127.0f));
        else
            macros.add(64);
    }
    return macros;
}

void PresetManager::setMacros(const juce::Array<int>& macros0to127)
{
    for (int i = 1; i <= 13; ++i)
    {
        const auto idx = i - 1;
        const int v = juce::jlimit(0, 127, idx < macros0to127.size() ? macros0to127[idx] : 64);
        if (auto* p = apvts.getParameter(macroParamId(i)))
            p->setValueNotifyingHost(float(v) / 127.0f);
    }
}

void PresetManager::applyBuiltinPreset(int index)
{
    const auto json = library.getPresetJson(index);
    const auto v = juce::JSON::parse(json);
    if (auto* obj = v.getDynamicObject())
    {
        const auto macrosVar = obj->getProperty("macros");
        if (auto* arr = macrosVar.getArray())
        {
            juce::Array<int> macros;
            macros.ensureStorageAllocated(13);
            for (int i = 0; i < juce::jmin(13, arr->size()); ++i)
            {
                const auto f = (float) arr->getReference(i);
                macros.add(juce::jlimit(0, 127, juce::roundToInt(juce::jlimit(0.0f, 1.0f, f) * 127.0f)));
            }
            setMacros(macros);
        }
    }
}

juce::File PresetManager::getUserSlotsDir() const
{
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("xAI Music Tools")
                   .getChildFile("MelodyForgePro")
                   .getChildFile("UserPresets");
    dir.createDirectory();
    return dir;
}

juce::File PresetManager::getUserSlotFile(int slotIndex) const
{
    const auto idx = juce::jlimit(0, 4, slotIndex);
    return getUserSlotsDir().getChildFile(juce::String::formatted("UserSlot%d.json", idx + 1));
}

bool PresetManager::saveUserSlot(int slotIndex)
{
    const auto file = getUserSlotFile(slotIndex);
    const auto macros = getCurrentMacros();

    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("name", juce::String::formatted("User Slot %d", slotIndex + 1));
    juce::Array<juce::var> arr;
    for (int i = 0; i < macros.size(); ++i)
        arr.add((double) juce::jlimit(0, 127, macros[i]) / 127.0);
    obj->setProperty("macros", juce::var(arr));

    const auto json = juce::JSON::toString(juce::var(obj), true);
    return file.replaceWithText(json);
}

bool PresetManager::loadUserSlot(int slotIndex)
{
    const auto file = getUserSlotFile(slotIndex);
    if (!file.existsAsFile())
        return false;

    const auto v = juce::JSON::parse(file);
    if (auto* obj = v.getDynamicObject())
    {
        const auto macrosVar = obj->getProperty("macros");
        if (auto* arr = macrosVar.getArray())
        {
            juce::Array<int> macros;
            macros.ensureStorageAllocated(13);
            for (int i = 0; i < juce::jmin(13, arr->size()); ++i)
            {
                const auto f = (float) arr->getReference(i);
                macros.add(juce::jlimit(0, 127, juce::roundToInt(juce::jlimit(0.0f, 1.0f, f) * 127.0f)));
            }
            setMacros(macros);
            return true;
        }
    }
    return false;
}
} // namespace mfpr

