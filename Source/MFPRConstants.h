#pragma once

#include "JuceIncludes.h"

namespace mfpr
{
inline constexpr auto kCompanyName = "xAI Music Tools";
inline constexpr auto kPluginName = "MelodyForgePro";
inline constexpr auto kPluginDisplayName = "MelodyForge Pro";

inline constexpr int kNumGenres = 32;
inline constexpr int kNumChordProgressions = 300;
inline constexpr int kNumMelodyShots = 200;
inline constexpr int kNumPresets = 200;
inline constexpr int kTotalEmbeddedAssets = 700;

inline const juce::Colour kBackground = juce::Colour(0xff1e1e1e);
inline const juce::Colour kAccent = juce::Colour(0xff00ff88);

inline const std::array<juce::String, kNumGenres> kGenres = {
    "House", "Deep House", "Tech House", "Progressive House",
    "Future House", "Bass House", "Electro House", "Big Room",
    "Techno", "Melodic Techno", "Minimal", "Trance",
    "Progressive Trance", "Hardstyle", "Dubstep", "Future Bass",
    "Drum & Bass", "Liquid DnB", "Garage", "UK Bass",
    "Trap", "Hip Hop", "R&B", "Pop",
    "Synthwave", "Disco", "Funk", "Ambient",
    "Lo-Fi", "EDM", "Tropical House", "Afro House"
};

inline const std::array<juce::String, 12> kKeys = { "C",  "C#", "D",  "D#", "E",  "F",
                                                    "F#", "G",  "G#", "A",  "A#", "B" };

inline const std::array<juce::String, 2> kModes = { "Major", "Minor" };
inline const std::array<juce::String, 4> kLengths = { "4", "8", "12", "16" };
inline const std::array<juce::String, 3> kTypes = { "Chord", "Melody", "Hybrid" };

inline constexpr int kEditorWidth = 800;
inline constexpr int kEditorHeight = 600;
} // namespace mfpr

