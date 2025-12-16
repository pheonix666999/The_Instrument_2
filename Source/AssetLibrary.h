#pragma once

#include "JuceIncludes.h"
#include "MelodyGenerator.h"
#include "MFPRConstants.h"

namespace mfpr
{
class AssetLibrary final
{
public:
    static constexpr int kChordCount = kNumChordProgressions;
    static constexpr int kMelodyCount = kNumMelodyShots;
    static constexpr int kPresetCount = kNumPresets;

    struct TemplateNote
    {
        int noteNumber = 60;
        int startStep = 0;
        int lengthSteps = 1;
        int velocity = 100;
    };

    struct TemplateSequence
    {
        int lengthSteps = 16;
        std::vector<TemplateNote> notes;
    };

    struct MatchResult
    {
        bool matched = false;
        bool isChord = false;
        int index = -1;      // 0..299 or 0..199 (depending on isChord)
        double score = 0.0;  // 0..1
    };

    AssetLibrary();

    const TemplateSequence& getChordTemplate(int index) const;
    const TemplateSequence& getMelodyTemplate(int index) const;

    juce::String getPresetJson(int index) const;
    juce::String getPresetName(int index) const;

    MatchResult matchMidiToLibrary(const juce::MidiMessageSequence& imported) const;

private:
    struct Features
    {
        std::array<float, 12> pitchClass = {};
        std::array<float, 64> rhythm = {};
    };

    static TemplateSequence parseEmbeddedMidiTemplate(const juce::String& resourceName);
    static TemplateSequence parseMidiSequenceTemplate(const juce::MidiMessageSequence& seq, int ticksPerQuarter);

    static Features computeFeatures(const TemplateSequence& tpl);
    static double similarity(const Features& a, const Features& b);

    std::array<TemplateSequence, kChordCount> chordTemplates;
    std::array<TemplateSequence, kMelodyCount> melodyTemplates;

    std::array<Features, kChordCount> chordFeatures;
    std::array<Features, kMelodyCount> melodyFeatures;

    std::array<juce::String, kPresetCount> presetNames;
};
} // namespace mfpr

