#pragma once

#include "JuceIncludes.h"
#include "AssetLibrary.h"

namespace mfpr
{
class PresetManager final
{
public:
    PresetManager(AssetLibrary& library, juce::AudioProcessorValueTreeState& apvts);

    int getNumPresets() const { return AssetLibrary::kPresetCount; }
    juce::String getPresetName(int index) const { return library.getPresetName(index); }

    void applyBuiltinPreset(int index);

    bool loadUserSlot(int slotIndex); // 0..4
    bool saveUserSlot(int slotIndex); // 0..4

    juce::File getUserSlotsDir() const;
    juce::File getUserSlotFile(int slotIndex) const;

private:
    AssetLibrary& library;
    juce::AudioProcessorValueTreeState& apvts;

    static juce::String macroParamId(int macroIndex); // 1..13
    juce::Array<int> getCurrentMacros() const;
    void setMacros(const juce::Array<int>& macros0to127);
};
} // namespace mfpr

