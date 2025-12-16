#pragma once

#include "JuceIncludes.h"
#include "AssetLibrary.h"
#include "FxChain.h"
#include "MelodyGenerator.h"
#include "PresetManager.h"
#include "SynthEngine.h"
#include <mutex>

namespace mfpr
{
class MelodyForgeProAudioProcessor final : public juce::AudioProcessor
{
public:
    MelodyForgeProAudioProcessor();
    ~MelodyForgeProAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    AssetLibrary& getAssetLibrary() { return assetLibrary; }
    PresetManager& getPresetManager() { return presetManager; }

    // UI helpers
    int getGenreIndex() const;
    int getKeyIndex() const;
    int getModeIndex() const;
    int getLengthBars() const;
    int getTypeIndex() const;

    void triggerGenerateFromUI();

    struct SamplerSlotInfo
    {
        juce::String label;
        double matchScore = 0.0;
    };

    void importMidiToSamplerSlot(int slotIndex, const juce::File& midiFile);
    SamplerSlotInfo getSamplerSlotInfo(int slotIndex) const;

    std::shared_ptr<const GeneratedPattern> getCurrentPattern() const;
    void setEditedPattern(GeneratedPattern edited);

    bool exportCurrentPatternToFile(const juce::File& file, int forceTracks = 0) const;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    GenerationParams readGenerationParams(uint32_t seed) const;
    void generateAndSwapPattern(int rootNote, int velocity, int channel, bool chordInput);

    void buildPatternMidi(juce::MidiBuffer& out,
                          const GeneratedPattern& pattern,
                          int patternStartStep,
                          int blockStartStep,
                          double blockStartPpq,
                          double samplesPerQuarter,
                          int numSamples,
                          double swingAmount,
                          juce::Random& rnd,
                          bool humanize) const;

    //==============================================================================
    juce::AudioProcessorValueTreeState apvts;
    AssetLibrary assetLibrary;
    PresetManager presetManager;

    MelodyGenerator generator;
    SynthEngine synth;
    FxChain fxChain;

    std::atomic<bool> pendingGenerate { false };
    std::atomic<uint32_t> seedCounter { 1u };

    std::atomic<int> lastRootNote { 60 };
    std::atomic<int> lastInputVelocity { 100 };
    std::atomic<int> lastOutputChannel { 1 };

    std::atomic<int> patternStartStep { 0 };
    std::atomic<bool> gateOpen { false };

    mutable std::mutex patternMutex;
    std::shared_ptr<const GeneratedPattern> currentPattern;

    struct SlotState
    {
        GeneratedPattern pattern;
        bool hasPattern = false;
        SamplerSlotInfo info;

        bool active = false;
        int startStep = 0;
        int channel = 1;
    };
    std::array<SlotState, 4> slots;

    double internalPpq = 0.0;
};
} // namespace mfpr

