#pragma once

#include "JuceIncludes.h"

namespace mfpr
{
struct SynthParams
{
    float oscMix = 0.5f;     // 0..1 (osc2 amount)
    float detune = 0.08f;    // 0..1
    float cutoff = 0.65f;    // 0..1
    float resonance = 0.18f; // 0..1 (light)

    float attack = 0.01f;  // seconds
    float decay = 0.20f;   // seconds
    float sustain = 0.75f; // 0..1
    float release = 0.25f; // seconds

    float masterGain = 0.80f; // 0..1
};

class SynthEngine final
{
public:
    SynthEngine();

    void prepare(double sampleRate, int samplesPerBlock, int numOutputChannels);
    void reset();

    void setParams(const SynthParams& p);
    void render(juce::AudioBuffer<float>& output, const juce::MidiBuffer& midi, int startSample, int numSamples);

private:
    class Sound final : public juce::SynthesiserSound
    {
    public:
        bool appliesToNote(int) override { return true; }
        bool appliesToChannel(int) override { return true; }
    };

    class Voice final : public juce::SynthesiserVoice
    {
    public:
        explicit Voice(const SynthParams& sharedParams);

        bool canPlaySound(juce::SynthesiserSound*) override;

        void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int currentPitchWheelPosition) override;
        void stopNote(float velocity, bool allowTailOff) override;
        void pitchWheelMoved(int) override {}
        void controllerMoved(int, int) override {}

        void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

        void setCurrentPlaybackSampleRate(double newRate) override;

    private:
        const SynthParams& params;

        double sampleRate = 44100.0;
        double phase1 = 0.0, phase2 = 0.0;
        double phaseDelta1 = 0.0, phaseDelta2 = 0.0;
        float level = 0.0f;

        float filterStateL = 0.0f, filterStateR = 0.0f;

        juce::ADSR adsr;
        juce::ADSR::Parameters adsrParams;

        static float oscSaw(double phase);
        static float oscSquare(double phase);
        float processOnePole(float x, float& state) const;
    };

    SynthParams params;
    juce::Synthesiser synth;
    int numChannels = 2;
};
} // namespace mfpr

