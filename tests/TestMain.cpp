#include <algorithm>
#include <atomic>
#include <unordered_set>

#include "../Source/AssetLibrary.h"
#include "../Source/MelodyGenerator.h"
#include "../Source/MidiExporter.h"
#include "../Source/PluginProcessor.h"

namespace
{
struct TestFailure : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

static void require(bool condition, const char* message)
{
    if (!condition)
        throw TestFailure(message);
}

static std::uint64_t hashPattern(const mfpr::GeneratedPattern& p)
{
    std::uint64_t h = 1469598103934665603ull;
    auto mix = [&](std::uint64_t v)
    {
        h ^= v;
        h *= 1099511628211ull;
    };

    mix((std::uint64_t) p.lengthSteps);
    mix((std::uint64_t) p.numTracks);
    for (const auto& n : p.notes)
    {
        mix((std::uint64_t) (n.noteNumber & 0x7f));
        mix((std::uint64_t) (n.startStep & 0xffff));
        mix((std::uint64_t) (n.lengthSteps & 0xffff));
        mix((std::uint64_t) (n.velocity & 0x7f));
        mix((std::uint64_t) (n.channel & 0x0f));
        mix((std::uint64_t) (n.track & 0x0f));
    }
    return h;
}

static int runChordGenValidation()
{
    mfpr::AssetLibrary library;
    mfpr::MelodyGenerator gen;

    mfpr::GenerationParams params;
    params.genreIndex = 0;
    params.keyIndex = 0;  // C
    params.modeIndex = 0; // Major
    params.lengthBars = 4;
    params.type = mfpr::GeneratorType::chord;
    params.velocitySensitivity = 80;
    params.swing = 0;
    params.seed = 42;

    const auto pattern = gen.generate(params, 60, 100, 1, library);
    require(pattern.lengthSteps == 64, "Chord pattern length must be 4 bars (64 steps).");
    require(pattern.numTracks >= 4, "Chord generation must use 4 tracks (4-voice chord).");

    std::array<bool, 4> tracksPresent { false, false, false, false };
    for (const auto& n : pattern.notes)
        if (n.track >= 0 && n.track < 4)
            tracksPresent[(size_t) n.track] = true;

    require(std::all_of(tracksPresent.begin(), tracksPresent.end(), [](bool b) { return b; }),
            "Chord generation must output notes on tracks 0-3.");

    return 0;
}

static int runMidiExportIntegrity()
{
    mfpr::AssetLibrary library;
    mfpr::MelodyGenerator gen;

    mfpr::GenerationParams params;
    params.genreIndex = 3;
    params.keyIndex = 9;  // A
    params.modeIndex = 1; // Minor
    params.lengthBars = 8;
    params.type = mfpr::GeneratorType::hybrid;
    params.seed = 77;

    const auto pattern = gen.generate(params, 57, 110, 1, library);

    mfpr::MidiExporter::Settings s;
    s.ticksPerQuarterNote = 960;
    s.bpm = 128.0;
    const auto bytes = mfpr::MidiExporter::renderPatternToMidiFileBytes(pattern, s);

    juce::MemoryInputStream mis(bytes.getData(), bytes.getSize(), false);
    juce::MidiFile mf;
    require(mf.readFrom(mis), "Exported MIDI must parse.");
    require(mf.getNumTracks() >= 1 && mf.getNumTracks() <= 16, "Exported MIDI must have 1-16 tracks.");

    bool hasNotes = false;
    for (int t = 0; t < mf.getNumTracks(); ++t)
    {
        auto* tr = mf.getTrack(t);
        if (tr == nullptr)
            continue;
        for (int i = 0; i < tr->getNumEvents(); ++i)
        {
            const auto& m = tr->getEventPointer(i)->message;
            if (m.isNoteOn())
            {
                hasNotes = true;
                break;
            }
        }
        if (hasNotes)
            break;
    }
    require(hasNotes, "Exported MIDI must contain note-on events.");
    return 0;
}

static int runUiFixedSize()
{
    juce::ScopedJuceInitialiser_GUI init;
    mfpr::MelodyForgeProAudioProcessor proc;
    std::unique_ptr<juce::AudioProcessorEditor> editor(proc.createEditor());
    require(editor != nullptr, "Editor must be created.");
    require(editor->getWidth() == 800, "Editor width must be 800.");
    require(editor->getHeight() == 600, "Editor height must be 600.");
    require(!editor->isResizable(), "Editor must be non-resizable.");
    return 0;
}

static int runPresetLoadTest()
{
    mfpr::MelodyForgeProAudioProcessor proc;
    auto& pm = proc.getPresetManager();

    auto* macro01 = dynamic_cast<juce::AudioParameterInt*>(proc.getAPVTS().getParameter("macro01"));
    require(macro01 != nullptr, "macro01 parameter must exist.");

    const auto before = macro01->get();
    pm.applyBuiltinPreset(0);
    const auto after = macro01->get();
    require(before != after, "Loading a preset must change macro01.");
    return 0;
}

static int runRandomizationVariance()
{
    mfpr::AssetLibrary library;
    mfpr::MelodyGenerator gen;

    mfpr::GenerationParams params;
    params.genreIndex = 10;
    params.keyIndex = 2;
    params.modeIndex = 0;
    params.lengthBars = 4;
    params.type = mfpr::GeneratorType::melody;
    params.velocitySensitivity = 50;

    const int trials = 100;
    std::unordered_set<std::uint64_t> unique;
    unique.reserve((size_t) trials);

    for (int i = 0; i < trials; ++i)
    {
        params.seed = (uint32_t) (1000 + i);
        const auto pattern = gen.generate(params, 60, 100, 1, library);
        unique.insert(hashPattern(pattern));
    }

    const double ratio = double(unique.size()) / double(trials);
    require(ratio > 0.95, "Randomization variance must exceed 95%.");
    return 0;
}

static int runByName(const juce::String& name)
{
    if (name == "chord_gen_validation")
        return runChordGenValidation();
    if (name == "midi_export_integrity")
        return runMidiExportIntegrity();
    if (name == "ui_fixed_size")
        return runUiFixedSize();
    if (name == "preset_load_test")
        return runPresetLoadTest();
    if (name == "randomization_variance")
        return runRandomizationVariance();

    throw TestFailure("Unknown test name.");
}
} // namespace

int main(int argc, char** argv)
{
    try
    {
        if (argc < 2)
            throw TestFailure("Expected test name argument.");

        return runByName(argv[1]);
    }
    catch (const TestFailure& e)
    {
        juce::Logger::writeToLog(juce::String("TEST FAILED: ") + e.what());
        return 1;
    }
    catch (const std::exception& e)
    {
        juce::Logger::writeToLog(juce::String("UNEXPECTED ERROR: ") + e.what());
        return 2;
    }
}
