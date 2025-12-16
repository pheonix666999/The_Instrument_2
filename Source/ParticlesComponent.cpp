#include "ParticlesComponent.h"

namespace mfpr
{
ParticlesComponent::ParticlesComponent()
{
    setInterceptsMouseClicks(false, false);
    particles.resize(64);
    for (auto& p : particles)
        resetParticle(p, rnd);
}

void ParticlesComponent::setEnabled(bool shouldAnimate)
{
    enabled = shouldAnimate;
    setVisible(enabled);
    if (enabled)
        startTimerHz(30);
    else
        stopTimer();
    repaint();
}

void ParticlesComponent::resetParticle(Particle& p, juce::Random& r)
{
    const auto w = (float) juce::jmax(1, getWidth());
    const auto h = (float) juce::jmax(1, getHeight());

    p.pos = { r.nextFloat() * w, r.nextFloat() * h };
    const auto angle = r.nextFloat() * juce::MathConstants<float>::twoPi;
    const auto speed = 8.0f + 26.0f * r.nextFloat();
    p.vel = { std::cos(angle) * speed, std::sin(angle) * speed };
    p.alpha = 0.04f + 0.12f * r.nextFloat();
    p.size = 1.0f + 2.0f * r.nextFloat();
}

void ParticlesComponent::timerCallback()
{
    if (!enabled)
        return;

    const auto w = (float) juce::jmax(1, getWidth());
    const auto h = (float) juce::jmax(1, getHeight());
    const float dt = 1.0f / 30.0f;

    for (auto& p : particles)
    {
        p.pos += p.vel * dt;
        if (p.pos.x < -10.0f || p.pos.x > w + 10.0f || p.pos.y < -10.0f || p.pos.y > h + 10.0f)
            resetParticle(p, rnd);
    }

    repaint();
}

void ParticlesComponent::paint(juce::Graphics& g)
{
    if (!enabled)
        return;

    g.setColour(kAccent.withAlpha(0.10f));
    g.fillAll(juce::Colours::transparentBlack);

    for (const auto& p : particles)
    {
        g.setColour(kAccent.withAlpha(p.alpha));
        g.fillEllipse(p.pos.x, p.pos.y, p.size, p.size);
    }
}
} // namespace mfpr

