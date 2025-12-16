#include "AssetLibrary.h"

#include "BinaryData.h"

namespace mfpr
{
static constexpr int stepsPerBar = 16;

static juce::String resourceNameForChord(int index)
{
    return juce::String::formatted("ChordProg_%03d_mid", index);
}

static juce::String resourceNameForMelody(int index)
{
    return juce::String::formatted("MelodyShot_%03d_mid", index);
}

static juce::String resourceNameForPreset(int index)
{
    return juce::String::formatted("Preset_%03d_json", index);
}

static int velocityTo127(const juce::MidiMessage& m)
{
    const auto v = m.getVelocity();
    const auto vf = float(v);
    if (vf <= 1.0f)
        return juce::jlimit(1, 127, juce::roundToInt(vf * 127.0f));
    return juce::jlimit(1, 127, juce::roundToInt(vf));
}

AssetLibrary::AssetLibrary()
{
    for (int i = 0; i < kChordCount; ++i)
        chordTemplates[(size_t) i] = parseEmbeddedMidiTemplate(resourceNameForChord(i));

    for (int i = 0; i < kMelodyCount; ++i)
        melodyTemplates[(size_t) i] = parseEmbeddedMidiTemplate(resourceNameForMelody(i));

    for (int i = 0; i < kChordCount; ++i)
        chordFeatures[(size_t) i] = computeFeatures(chordTemplates[(size_t) i]);

    for (int i = 0; i < kMelodyCount; ++i)
        melodyFeatures[(size_t) i] = computeFeatures(melodyTemplates[(size_t) i]);

    for (int i = 0; i < kPresetCount; ++i)
    {
        const auto json = getPresetJson(i);
        auto name = juce::String::formatted("Preset %03d", i);

        const auto v = juce::JSON::parse(json);
        if (auto* obj = v.getDynamicObject())
        {
            const auto n = obj->getProperty("name").toString();
            if (n.isNotEmpty())
                name = n;
        }

        presetNames[(size_t) i] = name;
    }
}

const AssetLibrary::TemplateSequence& AssetLibrary::getChordTemplate(int index) const
{
    return chordTemplates[(size_t) juce::jlimit(0, kChordCount - 1, index)];
}

const AssetLibrary::TemplateSequence& AssetLibrary::getMelodyTemplate(int index) const
{
    return melodyTemplates[(size_t) juce::jlimit(0, kMelodyCount - 1, index)];
}

juce::String AssetLibrary::getPresetJson(int index) const
{
    const auto resource = resourceNameForPreset(juce::jlimit(0, kPresetCount - 1, index));
    int size = 0;
    if (auto* data = BinaryData::getNamedResource(resource.toRawUTF8(), size))
    {
        juce::MemoryInputStream mis(data, (size_t) size, false);
        juce::GZIPDecompressorInputStream gzip(&mis, false, juce::GZIPDecompressorInputStream::gzipFormat);
        juce::MemoryOutputStream mos;
        mos.writeFromInputStream(gzip, -1);
        return juce::String::fromUTF8((const char*) mos.getData(), (int) mos.getDataSize());
    }

    return "{}";
}

juce::String AssetLibrary::getPresetName(int index) const
{
    return presetNames[(size_t) juce::jlimit(0, kPresetCount - 1, index)];
}

AssetLibrary::TemplateSequence AssetLibrary::parseEmbeddedMidiTemplate(const juce::String& resourceName)
{
    int size = 0;
    auto* data = BinaryData::getNamedResource(resourceName.toRawUTF8(), size);
    jassert(data != nullptr && size > 0);

    juce::MemoryInputStream mis(data, (size_t) size, false);
    juce::GZIPDecompressorInputStream gzip(&mis, false, juce::GZIPDecompressorInputStream::gzipFormat);
    juce::MidiFile midi;
    const bool ok = midi.readFrom(gzip);
    jassert(ok);

    const auto tpq = midi.getTimeFormat();
    auto* track = midi.getNumTracks() > 0 ? midi.getTrack(0) : nullptr;
    if (track == nullptr)
        return {};

    auto seq = *track;
    seq.updateMatchedPairs();
    return parseMidiSequenceTemplate(seq, tpq > 0 ? tpq : 960);
}

AssetLibrary::TemplateSequence AssetLibrary::parseMidiSequenceTemplate(const juce::MidiMessageSequence& seq, int ticksPerQuarter)
{
    TemplateSequence tpl;
    tpl.notes.reserve(256);

    const auto stepTicks = double(ticksPerQuarter) / 4.0;
    double maxEndTick = 0.0;

    for (int i = 0; i < seq.getNumEvents(); ++i)
    {
        auto* e = seq.getEventPointer(i);
        if (e == nullptr)
            continue;

        const auto& m = e->message;
        if (!m.isNoteOn())
            continue;

        if (auto* off = e->noteOffObject)
        {
            const auto startTick = m.getTimeStamp();
            const auto endTick = off->message.getTimeStamp();
            const auto startStep = int(std::llround(startTick / stepTicks));
            const auto lenSteps = juce::jmax(1, int(std::llround((endTick - startTick) / stepTicks)));

            TemplateNote tn;
            tn.noteNumber = m.getNoteNumber();
            tn.startStep = juce::jmax(0, startStep);
            tn.lengthSteps = lenSteps;
            tn.velocity = velocityTo127(m);
            tpl.notes.push_back(tn);

            maxEndTick = std::max(maxEndTick, endTick);
        }
    }

    const int maxStep = int(std::ceil(maxEndTick / stepTicks));
    tpl.lengthSteps = juce::jmax(stepsPerBar, ((maxStep + (stepsPerBar - 1)) / stepsPerBar) * stepsPerBar);
    return tpl;
}

AssetLibrary::Features AssetLibrary::computeFeatures(const TemplateSequence& tpl)
{
    Features f;

    for (auto& v : f.pitchClass)
        v = 0.0f;
    for (auto& v : f.rhythm)
        v = 0.0f;

    float total = 0.0f;
    for (const auto& n : tpl.notes)
    {
        f.pitchClass[(size_t) (n.noteNumber % 12)] += 1.0f;
        f.rhythm[(size_t) (n.startStep % int(f.rhythm.size()))] += 1.0f;
        total += 1.0f;
    }

    if (total <= 0.0f)
        return f;

    for (auto& v : f.pitchClass)
        v /= total;
    for (auto& v : f.rhythm)
        v /= total;

    return f;
}

double AssetLibrary::similarity(const Features& a, const Features& b)
{
    double pc = 0.0;
    for (size_t i = 0; i < a.pitchClass.size(); ++i)
        pc += std::min<double>(a.pitchClass[i], b.pitchClass[i]);

    double r = 0.0;
    for (size_t i = 0; i < a.rhythm.size(); ++i)
        r += std::min<double>(a.rhythm[i], b.rhythm[i]);

    pc = juce::jlimit(0.0, 1.0, pc);
    r = juce::jlimit(0.0, 1.0, r);
    return 0.55 * pc + 0.45 * r;
}

AssetLibrary::MatchResult AssetLibrary::matchMidiToLibrary(const juce::MidiMessageSequence& imported) const
{
    MatchResult best;
    best.score = 0.0;

    const auto features = computeFeatures(parseMidiSequenceTemplate(imported, 960));

    for (int i = 0; i < kChordCount; ++i)
    {
        const auto s = similarity(features, chordFeatures[(size_t) i]);
        if (s > best.score)
        {
            best.score = s;
            best.isChord = true;
            best.index = i;
        }
    }

    for (int i = 0; i < kMelodyCount; ++i)
    {
        const auto s = similarity(features, melodyFeatures[(size_t) i]);
        if (s > best.score)
        {
            best.score = s;
            best.isChord = false;
            best.index = i;
        }
    }

    best.matched = (best.score >= 0.85);
    return best;
}
} // namespace mfpr
