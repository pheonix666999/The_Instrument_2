#pragma once

#include "JuceIncludes.h"
#include "MFPRConstants.h"

namespace mfpr
{
class ParticlesComponent final : public juce::Component, private juce::Timer
{
public:
    ParticlesComponent();

    void setEnabled(bool shouldAnimate);

    void paint(juce::Graphics& g) override;
    void resized() override {}

private:
    struct Particle
    {
        juce::Point<float> pos;
        juce::Point<float> vel;
        float alpha = 0.0f;
        float size = 1.0f;
    };

    void timerCallback() override;
    void resetParticle(Particle& p, juce::Random& rnd);

    bool enabled = false;
    juce::Random rnd;
    std::vector<Particle> particles;
};
} // namespace mfpr

