#pragma once

#include "JuceIncludes.h"
#include "MFPRConstants.h"
#include "MelodyGenerator.h"

namespace mfpr
{
class MelodyForgeProAudioProcessor;

class PianoRollComponent final : public juce::Component, private juce::Timer
{
public:
    explicit PianoRollComponent(MelodyForgeProAudioProcessor& processor);
    ~PianoRollComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override {}

    void mouseDown(const juce::MouseEvent& e) override;

private:
    void timerCallback() override;

    int yToMidiNote(int y) const;
    int xToStep(int x) const;
    juce::Rectangle<float> noteToRect(const MidiNote& n, int minNote, int maxNote) const;

    MelodyForgeProAudioProcessor& processor;
    std::shared_ptr<const GeneratedPattern> pattern;

    int keyIndex = 0;
    int modeIndex = 0;
};
} // namespace mfpr

