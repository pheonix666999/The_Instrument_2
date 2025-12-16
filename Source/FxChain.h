#pragma once

#include "JuceIncludes.h"

namespace mfpr
{
struct FxParams
{
    float wet = 0.20f; // 0..1

    float autopanRateHz = 0.25f;
    float autopanDepth = 0.20f;

    float bitcrush = 0.0f; // 0..1

    float chorus = 0.10f; // 0..1
    float compressor = 0.10f; // 0..1

    float delaySeconds = 0.22f;
    float delayFeedback = 0.28f;

    float reverb = 0.12f; // 0..1

    float phaser = 0.10f; // 0..1
    float flanger = 0.08f; // 0..1

    float distortion = 0.06f; // 0..1
    float limiter = 0.0f; // 0..1

    float eq = 0.0f; // 0..1
    float filter = 0.0f; // 0..1
    float gate = 0.0f; // 0..1

    float tremolo = 0.0f; // 0..1
    float stereoWidth = 0.0f; // 0..1
    float saturator = 0.0f; // 0..1
};

class FxChain final
{
public:
    FxChain();

    void prepare(double sampleRate, int samplesPerBlock, int numChannels);
    void reset();

    void setParams(const FxParams& p) { params = p; }
    void process(juce::AudioBuffer<float>& buffer);

private:
    // Exactly 16 FX processors.
    struct AutoPan
    {
        void prepare(double sr) { sampleRate = sr; phase = 0.0f; }
        void process(juce::AudioBuffer<float>& b, float rateHz, float depth);
        double sampleRate = 44100.0;
        float phase = 0.0f;
    };

    struct Bitcrush
    {
        void prepare(double sr) { sampleRate = sr; counter = 0; heldL = heldR = 0.0f; }
        void process(juce::AudioBuffer<float>& b, float amount);
        double sampleRate = 44100.0;
        int counter = 0;
        float heldL = 0.0f, heldR = 0.0f;
    };

    struct ModDelay
    {
        void prepare(double sr, int maxDelaySamples);
        void reset();
        float processSample(float x, float delaySamples, float feedback);
        double sampleRate = 44100.0;
        std::vector<float> buffer;
        int writePos = 0;
    };

    struct Chorus
    {
        void prepare(double sr);
        void process(juce::AudioBuffer<float>& b, float amount);
        double sampleRate = 44100.0;
        float phase = 0.0f;
        ModDelay delay;
    };

    struct Compressor
    {
        void prepare(double sr) { sampleRate = sr; env = 0.0f; }
        void process(juce::AudioBuffer<float>& b, float amount);
        double sampleRate = 44100.0;
        float env = 0.0f;
    };

    struct Delay
    {
        void prepare(double sr);
        void reset() { delay.reset(); }
        void process(juce::AudioBuffer<float>& b, float seconds, float feedback);
        double sampleRate = 44100.0;
        ModDelay delay;
    };

    struct Reverb
    {
        void prepare(double sr) { sampleRate = sr; reverb.setSampleRate(sr); }
        void process(juce::AudioBuffer<float>& b, float amount);
        double sampleRate = 44100.0;
        juce::Reverb reverb;
    };

    struct Phaser
    {
        void prepare(double sr) { sampleRate = sr; phase = 0.0f; z1L = z1R = 0.0f; y1L = y1R = 0.0f; }
        void process(juce::AudioBuffer<float>& b, float amount);
        double sampleRate = 44100.0;
        float phase = 0.0f;
        float z1L = 0.0f, z1R = 0.0f;
        float y1L = 0.0f, y1R = 0.0f;
    };

    struct Flanger
    {
        void prepare(double sr);
        void process(juce::AudioBuffer<float>& b, float amount);
        double sampleRate = 44100.0;
        float phase = 0.0f;
        ModDelay delay;
    };

    struct Distortion
    {
        void process(juce::AudioBuffer<float>& b, float amount);
    };

    struct Limiter
    {
        void process(juce::AudioBuffer<float>& b, float amount);
    };

    struct Eq
    {
        void prepare(double sr) { sampleRate = sr; lpL = lpR = 0.0f; }
        void process(juce::AudioBuffer<float>& b, float amount);
        double sampleRate = 44100.0;
        float lpL = 0.0f, lpR = 0.0f;
    };

    struct Filter
    {
        void prepare(double sr) { sampleRate = sr; zL = zR = 0.0f; }
        void process(juce::AudioBuffer<float>& b, float amount);
        double sampleRate = 44100.0;
        float zL = 0.0f, zR = 0.0f;
    };

    struct Gate
    {
        void process(juce::AudioBuffer<float>& b, float amount);
    };

    struct Tremolo
    {
        void prepare(double sr) { sampleRate = sr; phase = 0.0f; }
        void process(juce::AudioBuffer<float>& b, float amount);
        double sampleRate = 44100.0;
        float phase = 0.0f;
    };

    struct StereoWidth
    {
        void process(juce::AudioBuffer<float>& b, float amount);
    };

    struct Saturator
    {
        void process(juce::AudioBuffer<float>& b, float amount);
    };

    FxParams params;
    int numChannels = 2;

    AutoPan fxAutoPan;
    Bitcrush fxBitcrush;
    Chorus fxChorus;
    Compressor fxCompressor;
    Delay fxDelay;
    Reverb fxReverb;
    Phaser fxPhaser;
    Flanger fxFlanger;
    Distortion fxDistortion;
    Limiter fxLimiter;
    Eq fxEq;
    Filter fxFilter;
    Gate fxGate;
    Tremolo fxTremolo;
    StereoWidth fxStereoWidth;
    Saturator fxSaturator;
};
} // namespace mfpr

