#pragma once

#include "JuceIncludes.h"
#include "MFPRConstants.h"

namespace mfpr
{
class MelodyForgeProAudioProcessor;

class SamplerSlotsComponent final : public juce::Component, public juce::FileDragAndDropTarget
{
public:
    explicit SamplerSlotsComponent(MelodyForgeProAudioProcessor& processor);

    void paint(juce::Graphics& g) override;
    void resized() override {}

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent&) override;

private:
    int slotAt(int x, int y) const;
    juce::Rectangle<int> getSlotBounds(int slot) const;

    MelodyForgeProAudioProcessor& processor;
    int hoveredSlot = -1;
};
} // namespace mfpr

