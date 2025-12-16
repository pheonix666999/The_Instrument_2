#include "MelodyGenerator.h"
#include "AssetLibrary.h"

namespace mfpr
{
static constexpr int stepsPerBar = 16;

static const std::array<int, 7> majorScale = { 0, 2, 4, 5, 7, 9, 11 };
static const std::array<int, 7> minorScale = { 0, 2, 3, 5, 7, 8, 10 };

static int positiveMod(int value, int mod)
{
    const int m = value % mod;
    return m < 0 ? (m + mod) : m;
}

static bool isPitchClassInScale(int pitchClass, int keyRoot, bool minor)
{
    const auto pc = positiveMod(pitchClass - keyRoot, 12);
    const auto& scale = minor ? minorScale : majorScale;
    return std::find(scale.begin(), scale.end(), pc) != scale.end();
}

static int snapToScale(int midiNote, int keyRoot, bool minor)
{
    if (isPitchClassInScale(midiNote % 12, keyRoot, minor))
        return juce::jlimit(0, 127, midiNote);

    for (int delta = 1; delta <= 2; ++delta)
    {
        if (isPitchClassInScale((midiNote + delta) % 12, keyRoot, minor))
            return juce::jlimit(0, 127, midiNote + delta);
        if (isPitchClassInScale((midiNote - delta) % 12, keyRoot, minor))
            return juce::jlimit(0, 127, midiNote - delta);
    }

    return juce::jlimit(0, 127, midiNote);
}

static int pickVelocityVariation(uint32_t seed)
{
    static constexpr std::array<int, 5> variations = { 40, 55, 70, 85, 100 };
    return variations[seed % variations.size()];
}

static int mixVelocity(int baseVelocity, int inputVelocity, int sensitivity0to100)
{
    const auto sens = juce::jlimit(0.0f, 1.0f, float(sensitivity0to100) / 100.0f);
    const auto v = juce::roundToInt((1.0f - sens) * float(baseVelocity) + sens * float(inputVelocity));
    return juce::jlimit(1, 127, v);
}

static void addNote(GeneratedPattern& out, int note, int startStep, int lengthSteps, int vel, int channel, int track)
{
    MidiNote n;
    n.noteNumber = juce::jlimit(0, 127, note);
    n.startStep = juce::jmax(0, startStep);
    n.lengthSteps = juce::jmax(1, lengthSteps);
    n.velocity = juce::jlimit(1, 127, vel);
    n.channel = juce::jlimit(1, 16, channel);
    n.track = juce::jlimit(0, 15, track);
    out.notes.push_back(n);
}

static void generateChordPart(GeneratedPattern& out,
                              const GenerationParams& params,
                              int rootNote,
                              int inputVelocity,
                              int channel,
                              AssetLibrary& library,
                              juce::Random& rnd)
{
    const bool minor = (params.modeIndex != 0);
    const int keyRoot = params.keyIndex;
    const int totalSteps = params.lengthBars * stepsPerBar;
    out.lengthSteps = totalSteps;

    const auto templateIndex = int(params.seed % uint32_t(AssetLibrary::kChordCount));
    const auto tpl = library.getChordTemplate(templateIndex);
    const auto baseVel = pickVelocityVariation(params.seed + 13u);
    const bool arpeggio = (params.genreIndex % 2) == 1 || rnd.nextBool();

    const std::array<int, 4> intervals = minor ? std::array<int, 4>{ 0, 3, 7, 10 } : std::array<int, 4>{ 0, 4, 7, 11 };

    for (int loopStart = 0; loopStart < totalSteps; loopStart += tpl.lengthSteps)
    {
        for (const auto& tn : tpl.notes)
        {
            const int start = loopStart + tn.startStep;
            if (start >= totalSteps)
                continue;

            const int pitchClassOffset = tn.noteNumber % 12; // file encodes degrees as pitch class offsets from C
            const int chordRoot = snapToScale(rootNote + pitchClassOffset, keyRoot, minor);
            const int vel = mixVelocity(baseVel, inputVelocity, params.velocitySensitivity);
            const int dur = juce::jlimit(1, totalSteps - start, tn.lengthSteps);

            if (!arpeggio)
            {
                for (int v = 0; v < 4; ++v)
                    addNote(out, snapToScale(chordRoot + intervals[v], keyRoot, minor), start, dur, vel, channel, v);
            }
            else
            {
                const int arpLen = juce::jmax(1, juce::jmin(4, dur));
                for (int v = 0; v < 4; ++v)
                    addNote(out,
                            snapToScale(chordRoot + intervals[v], keyRoot, minor),
                            start + (v % arpLen),
                            1,
                            vel,
                            channel,
                            v);
            }
        }
    }

    out.numTracks = 4;
}

static void generateMelodyPart(GeneratedPattern& out,
                               const GenerationParams& params,
                               int rootNote,
                               int inputVelocity,
                               int channel,
                               int melodyTrack,
                               AssetLibrary& library,
                               juce::Random& rnd,
                               std::function<juce::Array<int>(int step)> chordTonesAtStep)
{
    const bool minor = (params.modeIndex != 0);
    const int keyRoot = params.keyIndex;
    const int totalSteps = params.lengthBars * stepsPerBar;
    out.lengthSteps = totalSteps;

    const auto templateIndex = int((params.seed * 17u + 3u) % uint32_t(AssetLibrary::kMelodyCount));
    const auto tpl = library.getMelodyTemplate(templateIndex);

    int lastNote = snapToScale(rootNote, keyRoot, minor);
    int lastStep = -999;

    for (int loopStart = 0; loopStart < totalSteps; loopStart += tpl.lengthSteps)
    {
        for (const auto& tn : tpl.notes)
        {
            const int start = loopStart + tn.startStep;
            if (start >= totalSteps)
                continue;

            int note = rootNote + (tn.noteNumber - 60);
            note = snapToScale(note, keyRoot, minor);

            // Hybrid "AI": 80% (or caller-defined) chance to stick to chord tones at the step.
            if (chordTonesAtStep != nullptr && rnd.nextFloat() < params.melodyFollowChordChance)
            {
                const auto tones = chordTonesAtStep(start);
                if (tones.size() > 0)
                    note = tones[rnd.nextInt(tones.size())];
            }

            // 98% hit-worthy filter: avoid big consecutive leaps (> minor 9th).
            if (start != lastStep)
            {
                int diff = std::abs(note - lastNote);
                while (diff > 13)
                {
                    note += (note > lastNote) ? -12 : 12;
                    note = snapToScale(note, keyRoot, minor);
                    diff = std::abs(note - lastNote);
                    if (diff <= 13)
                        break;
                    note = lastNote + juce::jlimit(-13, 13, note - lastNote);
                    note = snapToScale(note, keyRoot, minor);
                    diff = std::abs(note - lastNote);
                }
            }

            // 5-15% velocity randomisation + velocity sensitivity blend
            const auto velRand = 0.05f + 0.10f * rnd.nextFloat();
            const auto rawVel = juce::roundToInt(float(tn.velocity) * (1.0f + (rnd.nextBool() ? velRand : -velRand)));
            const auto vel = mixVelocity(rawVel, inputVelocity, params.velocitySensitivity);

            const int dur = juce::jlimit(1, totalSteps - start, tn.lengthSteps);
            addNote(out, note, start, dur, vel, channel, melodyTrack);

            lastNote = note;
            lastStep = start;
        }
    }

    out.numTracks = juce::jmax(out.numTracks, melodyTrack + 1);
}

GeneratedPattern MelodyGenerator::generate(const GenerationParams& params,
                                          int inputRootMidiNote,
                                          int inputVelocity,
                                          int outputChannel,
                                          AssetLibrary& library) const
{
    GeneratedPattern out;
    out.notes.reserve(2048);

    juce::Random rnd(params.seed);
    const int root = juce::jlimit(0, 127, inputRootMidiNote);
    const int velIn = juce::jlimit(1, 127, inputVelocity);
    const int channel = juce::jlimit(1, 16, outputChannel);

    if (params.type == GeneratorType::chord)
    {
        generateChordPart(out, params, root, velIn, channel, library, rnd);
        out.numTracks = 4;
        return out;
    }

    if (params.type == GeneratorType::melody)
    {
        generateMelodyPart(out, params, root, velIn, channel, 0, library, rnd, nullptr);
        out.numTracks = 1;
        return out;
    }

    // Hybrid: chord + melody, with chord-tone adherence.
    GeneratedPattern chordPart;
    chordPart.notes.reserve(1024);
    generateChordPart(chordPart, params, root, velIn, channel, library, rnd);

    auto tonesAtStep = [minor = (params.modeIndex != 0), keyRoot = params.keyIndex, &chordPart](int step) -> juce::Array<int>
    {
        juce::Array<int> tones;
        for (const auto& n : chordPart.notes)
        {
            if (n.startStep == step)
            {
                const auto note = snapToScale(n.noteNumber, keyRoot, minor);
                if (!tones.contains(note))
                    tones.add(note);
            }
        }
        return tones;
    };

    out = chordPart;
    generateMelodyPart(out, params, root, velIn, channel, 4, library, rnd, tonesAtStep);
    out.numTracks = 5;
    return out;
}

double MelodyGenerator::score(const GeneratedPattern& pattern, const GenerationParams& params) const
{
    if (pattern.notes.empty())
        return -1.0;

    double s = 0.0;
    s += double(pattern.notes.size()) * 0.01;

    // Prefer some rhythmic density, but avoid floods.
    const double density = double(pattern.notes.size()) / juce::jmax(1, pattern.lengthSteps);
    s -= std::abs(density - (params.type == GeneratorType::melody ? 0.50 : 0.35)) * 2.0;

    // Penalise large melodic jumps on the melody track (if present).
    int lastMelNote = -1, lastStep = -999;
    for (const auto& n : pattern.notes)
    {
        if (n.track != 4 && params.type == GeneratorType::hybrid)
            continue;
        if (params.type == GeneratorType::melody && n.track != 0)
            continue;

        if (lastMelNote >= 0 && n.startStep != lastStep)
            s -= std::max(0, std::abs(n.noteNumber - lastMelNote) - 7) * 0.02;

        lastMelNote = n.noteNumber;
        lastStep = n.startStep;
    }

    return s;
}
} // namespace mfpr
