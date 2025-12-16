#include "FxChain.h"

namespace mfpr
{
static float softClip(float x)
{
    return std::tanh(x);
}

void FxChain::AutoPan::process(juce::AudioBuffer<float>& b, float rateHz, float depth)
{
    if (b.getNumChannels() < 2)
        return;
    if (depth <= 0.0001f || rateHz <= 0.001f)
        return;

    const auto inc = float(rateHz / sampleRate);
    auto* l = b.getWritePointer(0);
    auto* r = b.getWritePointer(1);
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        const auto pan = 0.5f + 0.5f * std::sin(juce::MathConstants<float>::twoPi * phase);
        phase += inc;
        if (phase >= 1.0f)
            phase -= 1.0f;

        const auto gL = 1.0f - depth * pan;
        const auto gR = 1.0f - depth * (1.0f - pan);
        l[i] *= gL;
        r[i] *= gR;
    }
}

void FxChain::Bitcrush::process(juce::AudioBuffer<float>& b, float amount)
{
    if (amount <= 0.0001f)
        return;

    const int bits = juce::jlimit(4, 16, 16 - int(amount * 12.0f));
    const float step = 1.0f / float(1 << bits);
    const int downsample = juce::jlimit(1, 12, 1 + int(amount * 11.0f));

    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        if ((counter++ % downsample) == 0)
        {
            heldL = std::round(b.getSample(0, i) / step) * step;
            heldR = b.getNumChannels() > 1 ? std::round(b.getSample(1, i) / step) * step : heldL;
        }

        b.setSample(0, i, heldL);
        if (b.getNumChannels() > 1)
            b.setSample(1, i, heldR);
    }
}

void FxChain::ModDelay::prepare(double sr, int maxDelaySamples)
{
    sampleRate = sr;
    buffer.assign((size_t) juce::jmax(1, maxDelaySamples), 0.0f);
    writePos = 0;
}

void FxChain::ModDelay::reset()
{
    std::fill(buffer.begin(), buffer.end(), 0.0f);
    writePos = 0;
}

float FxChain::ModDelay::processSample(float x, float delaySamples, float feedback)
{
    const int size = (int) buffer.size();
    const float ds = juce::jlimit(1.0f, float(size - 2), delaySamples);
    const int d0 = (int) ds;
    const float frac = ds - float(d0);

    int readPos0 = writePos - d0;
    while (readPos0 < 0)
        readPos0 += size;
    int readPos1 = readPos0 - 1;
    while (readPos1 < 0)
        readPos1 += size;

    const float y0 = buffer[(size_t) readPos0];
    const float y1 = buffer[(size_t) readPos1];
    const float y = y0 + frac * (y1 - y0);

    buffer[(size_t) writePos] = x + y * juce::jlimit(0.0f, 0.98f, feedback);
    if (++writePos >= size)
        writePos = 0;

    return y;
}

void FxChain::Chorus::prepare(double sr)
{
    sampleRate = sr;
    phase = 0.0f;
    delay.prepare(sr, int(sr * 0.060));
}

void FxChain::Chorus::process(juce::AudioBuffer<float>& b, float amount)
{
    if (amount <= 0.0001f)
        return;

    const float rateHz = juce::jmap(amount, 0.15f, 0.90f);
    const float depthSamples = juce::jmap(amount, 8.0f, 32.0f);
    const float baseDelaySamples = juce::jmap(amount, 12.0f, 28.0f);

    const float inc = float(rateHz / sampleRate);
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        const float lfo = std::sin(juce::MathConstants<float>::twoPi * phase);
        phase += inc;
        if (phase >= 1.0f)
            phase -= 1.0f;

        const float d = baseDelaySamples + depthSamples * 0.5f * (1.0f + lfo);
        for (int ch = 0; ch < b.getNumChannels(); ++ch)
        {
            const float x = b.getSample(ch, i);
            const float y = delay.processSample(x, d, 0.0f);
            b.setSample(ch, i, x * (1.0f - 0.35f * amount) + y * (0.35f * amount));
        }
    }
}

void FxChain::Compressor::process(juce::AudioBuffer<float>& b, float amount)
{
    if (amount <= 0.0001f)
        return;

    const float threshold = juce::jmap(amount, 0.30f, 0.12f);
    const float ratio = juce::jmap(amount, 1.2f, 5.0f);
    const float attack = 0.01f;
    const float release = 0.10f;

    const float aA = std::exp(-1.0f / (attack * float(sampleRate)));
    const float aR = std::exp(-1.0f / (release * float(sampleRate)));

    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        float x = 0.0f;
        for (int ch = 0; ch < b.getNumChannels(); ++ch)
            x = std::max(x, std::abs(b.getSample(ch, i)));

        const float target = x;
        env = (target > env) ? (aA * env + (1.0f - aA) * target) : (aR * env + (1.0f - aR) * target);

        float gain = 1.0f;
        if (env > threshold)
        {
            const float over = env / threshold;
            gain = std::pow(over, -(ratio - 1.0f) / ratio);
        }

        for (int ch = 0; ch < b.getNumChannels(); ++ch)
            b.setSample(ch, i, b.getSample(ch, i) * gain);
    }
}

void FxChain::Delay::prepare(double sr)
{
    sampleRate = sr;
    delay.prepare(sr, int(sr * 2.0));
}

void FxChain::Delay::process(juce::AudioBuffer<float>& b, float seconds, float feedback)
{
    if (seconds <= 0.001f)
        return;

    const float delaySamples = juce::jlimit(1.0f, float(delay.buffer.size() - 2), seconds * float(sampleRate));
    const float fb = juce::jlimit(0.0f, 0.95f, feedback);

    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        for (int ch = 0; ch < b.getNumChannels(); ++ch)
        {
            const float x = b.getSample(ch, i);
            const float y = delay.processSample(x, delaySamples, fb);
            b.setSample(ch, i, x + y * 0.30f);
        }
    }
}

void FxChain::Reverb::process(juce::AudioBuffer<float>& b, float amount)
{
    if (amount <= 0.0001f)
        return;

    juce::Reverb::Parameters p;
    p.roomSize = juce::jlimit(0.0f, 1.0f, 0.25f + 0.70f * amount);
    p.damping = 0.35f;
    p.wetLevel = 0.10f + 0.35f * amount;
    p.dryLevel = 1.0f - 0.10f * amount;
    p.width = 1.0f;
    p.freezeMode = 0.0f;
    reverb.setParameters(p);

    juce::AudioBuffer<float> tmp(b.getNumChannels(), b.getNumSamples());
    tmp.makeCopyOf(b);
    reverb.processStereo(tmp.getWritePointer(0), tmp.getNumChannels() > 1 ? tmp.getWritePointer(1) : tmp.getWritePointer(0), tmp.getNumSamples());
    b.makeCopyOf(tmp);
}

void FxChain::Phaser::process(juce::AudioBuffer<float>& b, float amount)
{
    if (amount <= 0.0001f || b.getNumChannels() < 2)
        return;

    const float rateHz = juce::jmap(amount, 0.08f, 0.55f);
    const float inc = float(rateHz / sampleRate);

    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        const float lfo = 0.5f + 0.5f * std::sin(juce::MathConstants<float>::twoPi * phase);
        phase += inc;
        if (phase >= 1.0f)
            phase -= 1.0f;

        const float freq = juce::jmap(lfo, 180.0f, 1800.0f);
        const float w = 2.0f * juce::MathConstants<float>::pi * freq / float(sampleRate);
        const float a = (1.0f - w) / (1.0f + w);

        auto* l = b.getWritePointer(0);
        auto* r = b.getWritePointer(1);

        const float inL = l[i];
        const float outL = -a * inL + z1L + a * y1L;
        z1L = inL;
        y1L = outL;

        const float inR = r[i];
        const float outR = -a * inR + z1R + a * y1R;
        z1R = inR;
        y1R = outR;

        l[i] = inL * 0.7f + outL * 0.3f * amount;
        r[i] = inR * 0.7f + outR * 0.3f * amount;
    }
}

void FxChain::Flanger::prepare(double sr)
{
    sampleRate = sr;
    phase = 0.0f;
    delay.prepare(sr, int(sr * 0.020));
}

void FxChain::Flanger::process(juce::AudioBuffer<float>& b, float amount)
{
    if (amount <= 0.0001f)
        return;

    const float rateHz = juce::jmap(amount, 0.10f, 0.80f);
    const float inc = float(rateHz / sampleRate);

    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        const float lfo = std::sin(juce::MathConstants<float>::twoPi * phase);
        phase += inc;
        if (phase >= 1.0f)
            phase -= 1.0f;

        const float d = 4.0f + (1.0f + lfo) * juce::jmap(amount, 1.0f, 6.0f);
        for (int ch = 0; ch < b.getNumChannels(); ++ch)
        {
            const float x = b.getSample(ch, i);
            const float y = delay.processSample(x, d, 0.15f + 0.25f * amount);
            b.setSample(ch, i, x + y * (0.25f * amount));
        }
    }
}

void FxChain::Distortion::process(juce::AudioBuffer<float>& b, float amount)
{
    if (amount <= 0.0001f)
        return;
    const float drive = 1.0f + 12.0f * amount;
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
    {
        auto* p = b.getWritePointer(ch);
        for (int i = 0; i < b.getNumSamples(); ++i)
            p[i] = softClip(p[i] * drive);
    }
}

void FxChain::Limiter::process(juce::AudioBuffer<float>& b, float amount)
{
    if (amount <= 0.0001f)
        return;
    const float lim = juce::jmap(amount, 0.98f, 0.85f);
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
    {
        auto* p = b.getWritePointer(ch);
        for (int i = 0; i < b.getNumSamples(); ++i)
            p[i] = juce::jlimit(-lim, lim, p[i]);
    }
}

void FxChain::Eq::process(juce::AudioBuffer<float>& b, float amount)
{
    if (amount <= 0.0001f)
        return;

    const float tilt = juce::jmap(amount, -6.0f, 6.0f); // dB-ish
    const float gLow = std::pow(10.0f, tilt / 20.0f);
    const float gHigh = std::pow(10.0f, -tilt / 20.0f);

    const float cutoffHz = 600.0f;
    const float rc = 1.0f / (juce::MathConstants<float>::twoPi * cutoffHz);
    const float dt = 1.0f / float(sampleRate);
    const float a = dt / (rc + dt);

    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        for (int ch = 0; ch < b.getNumChannels(); ++ch)
        {
            float& lp = (ch == 0 ? lpL : lpR);
            const float x = b.getSample(ch, i);
            lp += a * (x - lp);
            const float hp = x - lp;
            b.setSample(ch, i, lp * gLow + hp * gHigh);
        }
    }
}

void FxChain::Filter::process(juce::AudioBuffer<float>& b, float amount)
{
    if (amount <= 0.0001f)
        return;

    const float cutoffHz = juce::jmap(amount, 200.0f, 8000.0f);
    const float rc = 1.0f / (juce::MathConstants<float>::twoPi * cutoffHz);
    const float dt = 1.0f / float(sampleRate);
    const float a = dt / (rc + dt);

    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        for (int ch = 0; ch < b.getNumChannels(); ++ch)
        {
            float& z = (ch == 0 ? zL : zR);
            const float x = b.getSample(ch, i);
            z += a * (x - z);
            b.setSample(ch, i, z);
        }
    }
}

void FxChain::Gate::process(juce::AudioBuffer<float>& b, float amount)
{
    if (amount <= 0.0001f)
        return;

    const float threshold = juce::jmap(amount, 0.02f, 0.20f);
    const float reduction = juce::jmap(amount, 0.90f, 0.20f);
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
    {
        auto* p = b.getWritePointer(ch);
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            if (std::abs(p[i]) < threshold)
                p[i] *= reduction;
        }
    }
}

void FxChain::Tremolo::process(juce::AudioBuffer<float>& b, float amount)
{
    if (amount <= 0.0001f)
        return;

    const float rateHz = juce::jmap(amount, 0.30f, 8.0f);
    const float inc = float(rateHz / sampleRate);
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        const float lfo = 0.5f + 0.5f * std::sin(juce::MathConstants<float>::twoPi * phase);
        phase += inc;
        if (phase >= 1.0f)
            phase -= 1.0f;
        const float g = juce::jmap(lfo, 1.0f - 0.7f * amount, 1.0f);
        for (int ch = 0; ch < b.getNumChannels(); ++ch)
            b.setSample(ch, i, b.getSample(ch, i) * g);
    }
}

void FxChain::StereoWidth::process(juce::AudioBuffer<float>& b, float amount)
{
    if (amount <= 0.0001f || b.getNumChannels() < 2)
        return;

    const float w = juce::jmap(amount, 1.0f, 1.8f);
    auto* l = b.getWritePointer(0);
    auto* r = b.getWritePointer(1);
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        const float mid = 0.5f * (l[i] + r[i]);
        const float side = 0.5f * (l[i] - r[i]) * w;
        l[i] = mid + side;
        r[i] = mid - side;
    }
}

void FxChain::Saturator::process(juce::AudioBuffer<float>& b, float amount)
{
    if (amount <= 0.0001f)
        return;

    const float drive = 1.0f + 6.0f * amount;
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
    {
        auto* p = b.getWritePointer(ch);
        for (int i = 0; i < b.getNumSamples(); ++i)
            p[i] = softClip(p[i] * drive) * (1.0f - 0.15f * amount);
    }
}

FxChain::FxChain() = default;

void FxChain::prepare(double sampleRate, int samplesPerBlock, int numCh)
{
    numChannels = juce::jlimit(1, 2, numCh);
    juce::ignoreUnused(samplesPerBlock);

    fxAutoPan.prepare(sampleRate);
    fxBitcrush.prepare(sampleRate);
    fxChorus.prepare(sampleRate);
    fxCompressor.prepare(sampleRate);
    fxDelay.prepare(sampleRate);
    fxReverb.prepare(sampleRate);
    fxPhaser.prepare(sampleRate);
    fxFlanger.prepare(sampleRate);
    fxEq.prepare(sampleRate);
    fxFilter.prepare(sampleRate);
    fxTremolo.prepare(sampleRate);
}

void FxChain::reset()
{
    fxDelay.reset();
}

void FxChain::process(juce::AudioBuffer<float>& buffer)
{
    if (buffer.getNumSamples() <= 0)
        return;

    const float wet = juce::jlimit(0.0f, 1.0f, params.wet);
    if (wet <= 0.0001f)
        return;

    juce::AudioBuffer<float> dry(buffer.getNumChannels(), buffer.getNumSamples());
    dry.makeCopyOf(buffer);

    fxAutoPan.process(buffer, params.autopanRateHz, params.autopanDepth);
    fxBitcrush.process(buffer, params.bitcrush);
    fxChorus.process(buffer, params.chorus);
    fxCompressor.process(buffer, params.compressor);
    fxDelay.process(buffer, params.delaySeconds, params.delayFeedback);
    fxReverb.process(buffer, params.reverb);
    fxPhaser.process(buffer, params.phaser);
    fxFlanger.process(buffer, params.flanger);
    fxDistortion.process(buffer, params.distortion);
    fxLimiter.process(buffer, params.limiter);
    fxEq.process(buffer, params.eq);
    fxFilter.process(buffer, params.filter);
    fxGate.process(buffer, params.gate);
    fxTremolo.process(buffer, params.tremolo);
    fxStereoWidth.process(buffer, params.stereoWidth);
    fxSaturator.process(buffer, params.saturator);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        buffer.applyGain(ch, 0, buffer.getNumSamples(), wet);
        buffer.addFrom(ch, 0, dry, ch, 0, dry.getNumSamples(), 1.0f - wet);
    }
}
} // namespace mfpr

