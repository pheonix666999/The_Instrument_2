#pragma once

#include "JuceIncludes.h"
#include "LookAndFeel.h"
#include "MFPRConstants.h"
#include "PianoRollComponent.h"
#include "SamplerSlotsComponent.h"
#include "ParticlesComponent.h"

namespace mfpr
{
class MelodyForgeProAudioProcessor;

class MelodyForgeProAudioEditor final : public juce::AudioProcessorEditor,
                                        public juce::DragAndDropContainer,
                                        private juce::ListBoxModel,
                                        private juce::Timer
{
public:
    explicit MelodyForgeProAudioEditor(MelodyForgeProAudioProcessor&);
    ~MelodyForgeProAudioEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;

private:
    // ListBoxModel
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics&, int width, int height, bool rowIsSelected) override;
    void selectedRowsChanged(int lastRowSelected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent& e) override;

    void timerCallback() override;

    void configureComboBox(juce::ComboBox& cb);
    void configureMacroSlider(juce::Slider& s, const juce::String& name);

    void doExportDrag();
    void updateParticles();

    MelodyForgeProAudioProcessor& processor;
    mfpr::LookAndFeel lookAndFeel;

    // Controls 1-5
    juce::ComboBox genreBox, keyBox, modeBox, lengthBox, typeBox;

    // Control 6
    juce::TextButton generateButton { "Generate (10x)" };

    // Control 7
    PianoRollComponent pianoRoll;

    // Control 8
    juce::ListBox presetList { "Presets", this };

    // Control 9
    SamplerSlotsComponent samplerSlots;

    // Controls 10-13
    juce::Slider macro1, macro2, macro3, macro4;

    // Control 14
    juce::TextButton exportButton { "Export MIDI (Drag Out)" };

    // Control 15
    juce::ToggleButton animationToggle { "Randomize Animation" };
    ParticlesComponent particles;

    // Attachments
    using APVTS = juce::AudioProcessorValueTreeState;
    std::unique_ptr<APVTS::ComboBoxAttachment> genreAttach, keyAttach, modeAttach, lengthAttach, typeAttach;
    std::unique_ptr<APVTS::SliderAttachment> macro1Attach, macro2Attach, macro3Attach, macro4Attach;

    juce::Component leftActions;
    juce::Component macroArea;
};
} // namespace mfpr

