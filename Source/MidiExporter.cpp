#include "MidiExporter.h"

namespace mfpr
{
static void addTimeSigAndTempo(juce::MidiMessageSequence& track,
                               int numerator,
                               int denominator,
                               double bpm,
                               int ticksPerQuarter)
{
    const auto metaTimeSig = juce::MidiMessage::timeSignatureMetaEvent(numerator, denominator);
    track.addEvent(metaTimeSig, 0.0);

    const auto mpqn = int(60'000'000.0 / juce::jmax(1.0, bpm));
    const auto metaTempo = juce::MidiMessage::tempoMetaEvent(mpqn);
    track.addEvent(metaTempo, 0.0);

    track.updateMatchedPairs();
    juce::ignoreUnused(ticksPerQuarter);
}

static int toTicksFromSteps(int step, int ticksPerQuarter)
{
    const auto stepTicks = ticksPerQuarter / 4;
    return step * stepTicks;
}

juce::MemoryBlock MidiExporter::renderPatternToMidiFileBytes(const GeneratedPattern& pattern, const Settings& settings)
{
    const auto ticksPerQuarter = juce::jlimit(96, 9600, settings.ticksPerQuarterNote);

    const auto requestedTracks = settings.forceNumTracks > 0 ? settings.forceNumTracks : pattern.numTracks;
    const auto numTracks = juce::jlimit(1, 16, requestedTracks);

    juce::MidiFile midi;
    midi.setTicksPerQuarterNote(ticksPerQuarter);

    std::vector<juce::MidiMessageSequence> tracks;
    tracks.resize((size_t) numTracks);

    addTimeSigAndTempo(tracks[0], settings.timeSigNumerator, settings.timeSigDenominator, settings.bpm, ticksPerQuarter);

    for (const auto& n : pattern.notes)
    {
        const auto tr = juce::jlimit(0, numTracks - 1, n.track);
        const auto startTick = toTicksFromSteps(n.startStep, ticksPerQuarter);
        const auto endTick = toTicksFromSteps(n.startStep + juce::jmax(1, n.lengthSteps), ticksPerQuarter);

        auto on = juce::MidiMessage::noteOn(n.channel, n.noteNumber, (juce::uint8) juce::jlimit(1, 127, n.velocity));
        on.setTimeStamp((double) startTick);
        tracks[(size_t) tr].addEvent(on);

        auto off = juce::MidiMessage::noteOff(n.channel, n.noteNumber);
        off.setTimeStamp((double) endTick);
        tracks[(size_t) tr].addEvent(off);
    }

    for (auto& t : tracks)
    {
        t.updateMatchedPairs();
        midi.addTrack(t);
    }

    juce::MemoryBlock mb;
    juce::MemoryOutputStream mos(mb, false);
    midi.writeTo(mos);
    return mb;
}

bool MidiExporter::writePatternToFile(const GeneratedPattern& pattern, const juce::File& file, const Settings& settings)
{
    file.getParentDirectory().createDirectory();
    const auto bytes = renderPatternToMidiFileBytes(pattern, settings);
    return file.replaceWithData(bytes.getData(), bytes.getSize());
}
} // namespace mfpr

