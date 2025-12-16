#pragma once

#include "JuceIncludes.h"
#include "MelodyGenerator.h"

namespace mfpr
{
class MidiExporter final
{
public:
    struct Settings
    {
        int ticksPerQuarterNote = 960;
        double bpm = 128.0;
        int timeSigNumerator = 4;
        int timeSigDenominator = 4;

        // 0 => use pattern's inferred tracks (clamped 1..16)
        int forceNumTracks = 0;
    };

    static bool writePatternToFile(const GeneratedPattern& pattern, const juce::File& file, const Settings& settings);
    static juce::MemoryBlock renderPatternToMidiFileBytes(const GeneratedPattern& pattern, const Settings& settings);
};
} // namespace mfpr

