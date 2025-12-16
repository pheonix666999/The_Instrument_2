#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "MFPRConstants.h"
#include "MidiExporter.h"

namespace mfpr
{
static int choiceIndexToLengthBars(int lengthChoiceIndex)
{
    const int idx = juce::jlimit(0, int(mfpr::kLengths.size()) - 1, lengthChoiceIndex);
    return mfpr::kLengths[(size_t) idx].getIntValue();
}

static double swingDiscreteFrom0to50(int swing0to50)
{
    const auto v = juce::jlimit(0, 50, swing0to50);
    if (v < 17)
        return 0.0;
    if (v < 34)
        return 0.15;
    return 0.30;
}

static SynthParams synthParamsFromMacros(const std::array<int, 13>& macros)
{
    auto norm = [&](int idx) -> float { return juce::jlimit(0.0f, 1.0f, float(macros[(size_t) idx]) / 127.0f); };

    SynthParams p;
    p.oscMix = norm(0);
    p.detune = norm(1);
    p.cutoff = norm(2);
    p.resonance = norm(3);

    p.attack = juce::jmap(norm(4), 0.002f, 0.35f);
    p.decay = juce::jmap(norm(5), 0.01f, 0.90f);
    p.sustain = juce::jmap(norm(6), 0.0f, 1.0f);
    p.release = juce::jmap(norm(7), 0.02f, 1.60f);

    p.masterGain = juce::jmap(norm(8), 0.25f, 1.0f);
    return p;
}

static FxParams fxParamsFromMacros(const std::array<int, 13>& macros)
{
    auto norm = [&](int idx) -> float { return juce::jlimit(0.0f, 1.0f, float(macros[(size_t) idx]) / 127.0f); };

    FxParams p;
    p.wet = 0.10f + 0.45f * norm(8);
    p.autopanRateHz = 0.10f + 1.20f * norm(9);
    p.autopanDepth = 0.05f + 0.55f * norm(9);
    p.delaySeconds = juce::jmap(norm(10), 0.10f, 0.45f);
    p.delayFeedback = juce::jmap(norm(10), 0.18f, 0.55f);
    p.reverb = juce::jmap(norm(11), 0.0f, 0.45f);
    p.bitcrush = juce::jmap(norm(12), 0.0f, 0.75f);

    // Gentle always-on seasoning
    p.chorus = 0.10f * norm(9);
    p.compressor = 0.08f + 0.10f * norm(8);
    p.phaser = 0.08f * norm(11);
    p.flanger = 0.06f * norm(11);
    p.distortion = 0.06f * norm(12);
    p.limiter = 0.10f * norm(8);
    p.eq = (norm(8) - 0.5f) * 0.3f + 0.5f;
    p.filter = 0.10f * norm(2);
    p.gate = 0.05f * (1.0f - norm(8));
    p.tremolo = 0.05f * norm(9);
    p.stereoWidth = 0.25f * norm(11);
    p.saturator = 0.08f * norm(12);
    return p;
}

MelodyForgeProAudioProcessor::MelodyForgeProAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , apvts(*this, nullptr, "Parameters", createParameterLayout())
    , assetLibrary()
    , presetManager(assetLibrary, apvts)
{
    currentPattern = std::make_shared<GeneratedPattern>();
}

MelodyForgeProAudioProcessor::~MelodyForgeProAudioProcessor() = default;

const juce::String MelodyForgeProAudioProcessor::getName() const
{
    return mfpr::kPluginName;
}

bool MelodyForgeProAudioProcessor::acceptsMidi() const
{
    return true;
}

bool MelodyForgeProAudioProcessor::producesMidi() const
{
    return true;
}

bool MelodyForgeProAudioProcessor::isMidiEffect() const
{
    return false;
}

double MelodyForgeProAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MelodyForgeProAudioProcessor::getNumPrograms()
{
    return 1;
}

int MelodyForgeProAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MelodyForgeProAudioProcessor::setCurrentProgram(int) {}

const juce::String MelodyForgeProAudioProcessor::getProgramName(int)
{
    return {};
}

void MelodyForgeProAudioProcessor::changeProgramName(int, const juce::String&) {}

void MelodyForgeProAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    internalPpq = 0.0;

    synth.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    fxChain.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
}

void MelodyForgeProAudioProcessor::releaseResources() {}

bool MelodyForgeProAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

static std::array<int, 13> readMacroInts(const juce::AudioProcessorValueTreeState& apvts)
{
    std::array<int, 13> macros {};
    for (int i = 1; i <= 13; ++i)
    {
        const auto id = juce::String::formatted("macro%02d", i);
        if (auto* p = dynamic_cast<juce::AudioParameterInt*>(apvts.getParameter(id)))
            macros[(size_t) (i - 1)] = p->get();
        else
            macros[(size_t) (i - 1)] = 64;
    }
    return macros;
}

int MelodyForgeProAudioProcessor::getGenreIndex() const
{
    return (int) apvts.getRawParameterValue("genre")->load();
}

int MelodyForgeProAudioProcessor::getKeyIndex() const
{
    return (int) apvts.getRawParameterValue("key")->load();
}

int MelodyForgeProAudioProcessor::getModeIndex() const
{
    return (int) apvts.getRawParameterValue("mode")->load();
}

int MelodyForgeProAudioProcessor::getLengthBars() const
{
    return choiceIndexToLengthBars((int) apvts.getRawParameterValue("length")->load());
}

int MelodyForgeProAudioProcessor::getTypeIndex() const
{
    return (int) apvts.getRawParameterValue("type")->load();
}

void MelodyForgeProAudioProcessor::triggerGenerateFromUI()
{
    pendingGenerate.store(true);
}

juce::AudioProcessorEditor* MelodyForgeProAudioProcessor::createEditor()
{
    return new MelodyForgeProAudioEditor(*this);
}

bool MelodyForgeProAudioProcessor::hasEditor() const
{
    return true;
}

std::shared_ptr<const GeneratedPattern> MelodyForgeProAudioProcessor::getCurrentPattern() const
{
    std::lock_guard<std::mutex> lock(patternMutex);
    return currentPattern;
}

void MelodyForgeProAudioProcessor::setEditedPattern(GeneratedPattern edited)
{
    edited.lengthSteps = juce::jmax(16, edited.lengthSteps);
    edited.numTracks = juce::jlimit(1, 16, edited.numTracks);
    std::lock_guard<std::mutex> lock(patternMutex);
    currentPattern = std::make_shared<GeneratedPattern>(std::move(edited));
}

GenerationParams MelodyForgeProAudioProcessor::readGenerationParams(uint32_t seed) const
{
    GenerationParams p;
    p.genreIndex = getGenreIndex();
    p.keyIndex = getKeyIndex();
    p.modeIndex = getModeIndex();
    p.lengthBars = getLengthBars();
    p.type = (GeneratorType) getTypeIndex();
    p.velocitySensitivity = (int) apvts.getRawParameterValue("velSens")->load();
    p.swing = (int) apvts.getRawParameterValue("swing")->load();
    p.seed = seed;
    p.melodyFollowChordChance = (p.type == GeneratorType::hybrid) ? 0.80f : 0.0f;
    return p;
}

void MelodyForgeProAudioProcessor::generateAndSwapPattern(int rootNote, int velocity, int channel, bool chordInput)
{
    auto baseSeed = seedCounter.fetch_add(1u);

    auto params = readGenerationParams(baseSeed);
    if (chordInput)
    {
        params.type = GeneratorType::hybrid;
        params.melodyFollowChordChance = 0.70f;
    }

    // Button triggers 10 variations: pick best-scoring.
    double bestScore = -1.0e9;
    GeneratedPattern best;

    for (uint32_t i = 0; i < 10; ++i)
    {
        params.seed = baseSeed + i;
        auto candidate = generator.generate(params, rootNote, velocity, channel, assetLibrary);
        const auto s = generator.score(candidate, params);
        if (s > bestScore)
        {
            bestScore = s;
            best = std::move(candidate);
        }
    }

    {
        std::lock_guard<std::mutex> lock(patternMutex);
        currentPattern = std::make_shared<GeneratedPattern>(std::move(best));
    }
    gateOpen.store(true);
}

static int floorDiv(int a, int b)
{
    jassert(b > 0);
    int q = a / b;
    int r = a % b;
    if ((r != 0) && ((r < 0) != (b < 0)))
        --q;
    return q;
}

void MelodyForgeProAudioProcessor::buildPatternMidi(juce::MidiBuffer& out,
                                                   const GeneratedPattern& pattern,
                                                   int patStartStep,
                                                   int blockStartStep,
                                                   double blockStartPpq,
                                                   double samplesPerQuarter,
                                                   int numSamples,
                                                   double swingAmount,
                                                   juce::Random& rnd,
                                                   bool humanize) const
{
    const int blockEndStep = blockStartStep + int(std::ceil((double) numSamples / (samplesPerQuarter / 4.0))) + 1;
    const int L = juce::jmax(1, pattern.lengthSteps);

    const int maxJitterSamples = humanize ? int(std::round(getSampleRate() * 0.010)) : 0; // ±10ms

    auto addEventAtStep = [&](const juce::MidiMessage& msg, int step, bool applyJitter)
    {
        const double eventPpq = double(step) / 4.0;
        const double deltaPpq = eventPpq - blockStartPpq;
        int samplePos = (int) std::llround(deltaPpq * samplesPerQuarter);
        if (applyJitter && maxJitterSamples > 0)
            samplePos += rnd.nextInt(juce::Range<int>(-maxJitterSamples, maxJitterSamples + 1));

        if ((step & 1) == 1) // swing off-steps
            samplePos += int(std::llround((samplesPerQuarter / 4.0) * swingAmount));

        samplePos = juce::jlimit(0, numSamples - 1, samplePos);
        out.addEvent(msg, samplePos);
    };

    for (const auto& n : pattern.notes)
    {
        const int baseOn = patStartStep + n.startStep;
        const int baseOff = baseOn + juce::jmax(1, n.lengthSteps);

        const int kOnStart = floorDiv(blockStartStep - baseOn, L);
        for (int k = kOnStart; k < kOnStart + 4; ++k)
        {
            const int onStep = baseOn + k * L;
            if (onStep < blockStartStep || onStep >= blockEndStep)
                continue;

            auto on = juce::MidiMessage::noteOn(n.channel, n.noteNumber, (juce::uint8) juce::jlimit(1, 127, n.velocity));
            addEventAtStep(on, onStep, humanize);
        }

        const int kOffStart = floorDiv(blockStartStep - baseOff, L);
        for (int k = kOffStart; k < kOffStart + 4; ++k)
        {
            const int offStep = baseOff + k * L;
            if (offStep < blockStartStep || offStep >= blockEndStep)
                continue;

            auto off = juce::MidiMessage::noteOff(n.channel, n.noteNumber);
            addEventAtStep(off, offStep, false);
        }
    }
}

void MelodyForgeProAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    // Sample-rate support (host-controlled): validated defensively.
    const auto sr = getSampleRate();
    if (sr > 0.0 && (sr < 44100.0 || sr > 192000.0))
        return;

    auto macros = readMacroInts(apvts);
    synth.setParams(synthParamsFromMacros(macros));
    fxChain.setParams(fxParamsFromMacros(macros));

    double bpm = 120.0;
    double blockStartPpq = internalPpq;
    bool havePos = false;

    if (auto* ph = getPlayHead())
    {
        if (auto posInfo = ph->getPosition())
        {
            if (auto bpmOpt = posInfo->getBpm())
            {
                bpm = *bpmOpt;
                havePos = true;
            }
            if (auto ppqOpt = posInfo->getPpqPosition())
            {
                blockStartPpq = *ppqOpt;
            }
        }
    }

    const double samplesPerQuarter = (sr > 0.0) ? (sr * 60.0 / bpm) : 22050.0;
    const double blockPpqLen = double(numSamples) / samplesPerQuarter;
    const double blockEndPpq = blockStartPpq + blockPpqLen;
    internalPpq = blockEndPpq;

    const int blockStartStep = int(std::floor(blockStartPpq * 4.0));

    // Pending generate from UI thread.
    if (pendingGenerate.exchange(false))
    {
        const int root = lastRootNote.load();
        const int vel = lastInputVelocity.load();
        const int ch = lastOutputChannel.load();
        generateAndSwapPattern(root, vel, ch, false);
        patternStartStep.store(blockStartStep);
    }

    // Detect incoming triggers.
    struct NoteOn { int note = 60; int vel = 100; int ch = 1; int samplePos = 0; };
    std::vector<NoteOn> noteOns;
    noteOns.reserve(16);

    for (const auto metadata : midiMessages)
    {
        const auto m = metadata.getMessage();
        if (m.isNoteOn())
        {
            noteOns.push_back({ m.getNoteNumber(), (int) m.getVelocity(), m.getChannel(), metadata.samplePosition });
        }
    }

    const auto isChordInput = [&]()
    {
        if ((int) noteOns.size() < 4)
            return false;
        const int first = noteOns.front().samplePos;
        int near = 0;
        for (const auto& n : noteOns)
            if (std::abs(n.samplePos - first) <= 64)
                ++near;
        return near >= 4;
    }();

    if (!noteOns.empty())
    {
        const int ch = noteOns.front().ch;
        int root = noteOns.front().note;
        int vel = noteOns.front().vel;

        if (isChordInput)
        {
            root = 127;
            vel = 0;
            for (const auto& n : noteOns)
            {
                root = std::min(root, n.note);
                vel = std::max(vel, n.vel);
            }
        }

        lastRootNote.store(root);
        lastInputVelocity.store(vel);
        lastOutputChannel.store(ch);

        generateAndSwapPattern(root, vel, ch, isChordInput);
        patternStartStep.store(blockStartStep);
    }

    // Build generated MIDI and merge.
    juce::MidiBuffer generated;
    juce::Random rnd(seedCounter.load());

    const auto swing = swingDiscreteFrom0to50((int) apvts.getRawParameterValue("swing")->load());
    const bool humanize = true;

    if (gateOpen.load())
    {
        if (auto pat = getCurrentPattern())
            buildPatternMidi(generated,
                             *pat,
                             patternStartStep.load(),
                             blockStartStep,
                             blockStartPpq,
                             samplesPerQuarter,
                             numSamples,
                             swing,
                             rnd,
                             humanize);
    }

    // Sampler slot one-shots (trigger notes 60..63).
    for (auto metadata : midiMessages)
    {
        const auto m = metadata.getMessage();
        if (!m.isNoteOn())
            continue;
        const int slot = m.getNoteNumber() - 60;
        if (slot < 0 || slot >= 4)
            continue;
        if (!slots[(size_t) slot].hasPattern)
            continue;

        slots[(size_t) slot].active = true;
        slots[(size_t) slot].startStep = blockStartStep;
        slots[(size_t) slot].channel = m.getChannel();
    }

    for (auto& s : slots)
    {
        if (!s.active || !s.hasPattern)
            continue;

        auto pat = s.pattern;
        for (auto& n : pat.notes)
            n.channel = s.channel;

        buildPatternMidi(generated,
                         pat,
                         s.startStep,
                         blockStartStep,
                         blockStartPpq,
                         samplesPerQuarter,
                         numSamples,
                         0.0,
                         rnd,
                         true);

        const int endStep = s.startStep + pat.lengthSteps + 1;
        if (blockStartStep > endStep)
            s.active = false;
    }

    midiMessages.addEvents(generated, 0, numSamples, 0);

    // Audio render
    synth.render(buffer, midiMessages, 0, numSamples);
    fxChain.process(buffer);
}

void MelodyForgeProAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream mos(destData, false);
    apvts.state.writeToStream(mos);
}

void MelodyForgeProAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData(data, (size_t) sizeInBytes);
    if (tree.isValid())
        apvts.replaceState(tree);
}

bool MelodyForgeProAudioProcessor::exportCurrentPatternToFile(const juce::File& file, int forceTracks) const
{
    const auto pat = getCurrentPattern();
    if (pat == nullptr)
        return false;

    MidiExporter::Settings s;
    s.bpm = 128.0;
    s.forceNumTracks = forceTracks;
    return MidiExporter::writePatternToFile(*pat, file, s);
}

void MelodyForgeProAudioProcessor::importMidiToSamplerSlot(int slotIndex, const juce::File& midiFile)
{
    const int idx = juce::jlimit(0, 3, slotIndex);
    if (!midiFile.existsAsFile())
        return;

    juce::FileInputStream in(midiFile);
    if (!in.openedOk())
        return;

    juce::MidiFile midi;
    if (!midi.readFrom(in))
        return;

    if (midi.getNumTracks() <= 0)
        return;

    auto* tr = midi.getTrack(0);
    if (tr == nullptr)
        return;

    juce::MidiMessageSequence seq(*tr);
    seq.updateMatchedPairs();

    auto match = assetLibrary.matchMidiToLibrary(seq);

    // Convert sequence -> GeneratedPattern (16th-quantised)
    GeneratedPattern pat;
    pat.notes.reserve(256);
    const int tpq = midi.getTimeFormat() > 0 ? midi.getTimeFormat() : 960;
    const double stepTicks = double(tpq) / 4.0;
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
            const int startStep = int(std::llround(startTick / stepTicks));
            const int lenSteps = juce::jmax(1, int(std::llround((endTick - startTick) / stepTicks)));
            pat.notes.push_back(MidiNote{ m.getNoteNumber(), startStep, lenSteps, juce::jlimit(1, 127, (int) m.getVelocity()), 1, 0 });
            maxEndTick = std::max(maxEndTick, endTick);
        }
    }

    const int maxStep = int(std::ceil(maxEndTick / stepTicks));
    pat.lengthSteps = juce::jmax(16, ((maxStep + 15) / 16) * 16);
    pat.numTracks = 1;

    auto& s = slots[(size_t) idx];
    s.pattern = std::move(pat);
    s.hasPattern = true;

    s.info.label = midiFile.getFileName();
    s.info.matchScore = match.matched ? match.score : 0.0;
    if (match.matched)
    {
        s.info.label = match.isChord ? juce::String::formatted("Matched Chords %03d", match.index)
                                     : juce::String::formatted("Matched Melody %03d", match.index);
    }
}

MelodyForgeProAudioProcessor::SamplerSlotInfo MelodyForgeProAudioProcessor::getSamplerSlotInfo(int slotIndex) const
{
    const int idx = juce::jlimit(0, 3, slotIndex);
    return slots[(size_t) idx].hasPattern ? slots[(size_t) idx].info : SamplerSlotInfo{ "Empty — drop MIDI", 0.0 };
}

juce::AudioProcessorValueTreeState::ParameterLayout MelodyForgeProAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterChoice>("genre", "Genre", juce::StringArray(mfpr::kGenres.data(), (int) mfpr::kGenres.size()), 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>("key", "Key", juce::StringArray(mfpr::kKeys.data(), (int) mfpr::kKeys.size()), 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>("mode", "Mode", juce::StringArray(mfpr::kModes.data(), (int) mfpr::kModes.size()), 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>("length", "Length", juce::StringArray(mfpr::kLengths.data(), (int) mfpr::kLengths.size()), 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>("type", "Type", juce::StringArray(mfpr::kTypes.data(), (int) mfpr::kTypes.size()), 0));

    layout.add(std::make_unique<juce::AudioParameterInt>("velSens", "Velocity Sensitivity", 0, 100, 80));
    layout.add(std::make_unique<juce::AudioParameterInt>("swing", "Swing", 0, 50, 0));

    for (int i = 1; i <= 13; ++i)
        layout.add(std::make_unique<juce::AudioParameterInt>(juce::String::formatted("macro%02d", i),
                                                            juce::String::formatted("Macro %d", i),
                                                            0,
                                                            127,
                                                            64));

    return layout;
}
} // namespace mfpr

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new mfpr::MelodyForgeProAudioProcessor();
}
