#pragma once

#include "JuceIncludes.h"

namespace mfpr
{
enum class GeneratorType
{
    chord = 0,
    melody = 1,
    hybrid = 2
};

struct MidiNote
{
    int noteNumber = 60;  // 0..127
    int startStep = 0;    // 16th-note steps
    int lengthSteps = 1;  // >= 1
    int velocity = 100;   // 1..127
    int channel = 1;      // 1..16
    int track = 0;        // 0..15
};

struct GeneratedPattern
{
    int lengthSteps = 64;  // 4 bars @ 16 steps/bar
    int numTracks = 1;     // 1..16
    std::vector<MidiNote> notes;
};

struct GenerationParams
{
    int genreIndex = 0;             // 0..31
    int keyIndex = 0;               // 0..11 (C..B)
    int modeIndex = 0;              // 0=Major, 1=Minor
    int lengthBars = 4;             // 4/8/12/16
    GeneratorType type = GeneratorType::chord;
    int velocitySensitivity = 80;   // 0..100
    int swing = 0;                  // 0..50 (quantised later)
    uint32_t seed = 1;

    // Hybrid rules
    float melodyFollowChordChance = 0.80f; // 0..1
};

class AssetLibrary;

class MelodyGenerator final
{
public:
    GeneratedPattern generate(const GenerationParams& params,
                              int inputRootMidiNote,
                              int inputVelocity,
                              int outputChannel,
                              AssetLibrary& library) const;

    double score(const GeneratedPattern& pattern, const GenerationParams& params) const;
};
} // namespace mfpr

