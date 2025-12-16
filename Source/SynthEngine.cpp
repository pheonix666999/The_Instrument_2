#include "SynthEngine.h"

namespace mfpr
{
static double midiNoteToHz(int midiNote)
{
    return 440.0 * std::pow(2.0, (double(midiNote) - 69.0) / 12.0);
}

SynthEngine::Voice::Voice(const SynthParams& sharedParams) : params(sharedParams)
{
    adsr.setSampleRate(sampleRate);
}

bool SynthEngine::Voice::canPlaySound(juce::SynthesiserSound* s)
{
    return dynamic_cast<Sound*>(s) != nullptr;
}

void SynthEngine::Voice::setCurrentPlaybackSampleRate(double newRate)
{
    sampleRate = (newRate > 0.0 ? newRate : 44100.0);
    adsr.setSampleRate(sampleRate);
}

float SynthEngine::Voice::oscSaw(double phase)
{
    // phase: 0..1
    return float(2.0 * phase - 1.0);
}

float SynthEngine::Voice::oscSquare(double phase)
{
    return phase < 0.5 ? 1.0f : -1.0f;
}

float SynthEngine::Voice::processOnePole(float x, float& state) const
{
    const auto cutoffHz = juce::jmap(params.cutoff, 0.0f, 1.0f, 80.0f, 16000.0f);
    const auto rc = 1.0f / (juce::MathConstants<float>::twoPi * cutoffHz);
    const auto dt = 1.0f / float(sampleRate);
    const auto alpha = dt / (rc + dt);
    state += alpha * (x - state);
    return state;
}

void SynthEngine::Voice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int)
{
    const auto hz = midiNoteToHz(midiNoteNumber);
    const auto detuneCents = juce::jmap(params.detune, 0.0f, 1.0f, 0.0f, 25.0f);
    const auto detuneRatio = std::pow(2.0, double(detuneCents) / 1200.0);

    phaseDelta1 = hz / sampleRate;
    phaseDelta2 = (hz * detuneRatio) / sampleRate;
    phase1 = phase2 = 0.0;

    level = juce::jlimit(0.0f, 1.0f, velocity);

    adsrParams.attack = juce::jlimit(0.001f, 2.0f, params.attack);
    adsrParams.decay = juce::jlimit(0.001f, 2.0f, params.decay);
    adsrParams.sustain = juce::jlimit(0.0f, 1.0f, params.sustain);
    adsrParams.release = juce::jlimit(0.001f, 5.0f, params.release);
    adsr.setParameters(adsrParams);
    adsr.noteOn();
}

void SynthEngine::Voice::stopNote(float, bool allowTailOff)
{
    adsr.noteOff();
    if (!allowTailOff || !adsr.isActive())
        clearCurrentNote();
}

void SynthEngine::Voice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (!isVoiceActive())
        return;

    auto* left = outputBuffer.getWritePointer(0, startSample);
    auto* right = outputBuffer.getNumChannels() > 1 ? outputBuffer.getWritePointer(1, startSample) : nullptr;

    const auto mix = juce::jlimit(0.0f, 1.0f, params.oscMix);
    const auto gain = juce::jlimit(0.0f, 1.0f, params.masterGain) * 0.25f;

    for (int i = 0; i < numSamples; ++i)
    {
        const float env = adsr.getNextSample();
        if (!adsr.isActive())
        {
            clearCurrentNote();
            break;
        }

        const float s1 = oscSaw(phase1);
        const float s2 = oscSquare(phase2);
        const float s = (1.0f - mix) * s1 + mix * s2;

        phase1 += phaseDelta1;
        phase2 += phaseDelta2;
        if (phase1 >= 1.0)
            phase1 -= 1.0;
        if (phase2 >= 1.0)
            phase2 -= 1.0;

        float y = s * level * env * gain;
        y = processOnePole(y, filterStateL);
        left[i] += y;
        if (right != nullptr)
            right[i] += processOnePole(y, filterStateR);
    }
}

SynthEngine::SynthEngine()
{
    for (int i = 0; i < 16; ++i)
        synth.addVoice(new Voice(params));
    synth.addSound(new Sound());
}

void SynthEngine::prepare(double sampleRate, int samplesPerBlock, int numOutputChannels)
{
    numChannels = juce::jlimit(1, 2, numOutputChannels);
    synth.setCurrentPlaybackSampleRate(sampleRate);
    juce::ignoreUnused(samplesPerBlock);
    reset();
}

void SynthEngine::reset()
{
    for (int ch = 1; ch <= 16; ++ch)
        synth.allNotesOff(ch, false);
}

void SynthEngine::setParams(const SynthParams& p)
{
    params = p;
}

void SynthEngine::render(juce::AudioBuffer<float>& output, const juce::MidiBuffer& midi, int startSample, int numSamples)
{
    synth.renderNextBlock(output, midi, startSample, numSamples);
}
} // namespace mfpr
