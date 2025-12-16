#include "LookAndFeel.h"

namespace mfpr
{
LookAndFeel::LookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, kBackground);

    setColour(juce::ComboBox::backgroundColourId, kBackground.brighter(0.08f));
    setColour(juce::ComboBox::outlineColourId, kAccent.withAlpha(0.55f));
    setColour(juce::ComboBox::textColourId, juce::Colours::white);
    setColour(juce::PopupMenu::backgroundColourId, kBackground.brighter(0.06f));

    setColour(juce::Slider::thumbColourId, kAccent);
    setColour(juce::Slider::rotarySliderFillColourId, kAccent);
    setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::white.withAlpha(0.15f));
    setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    setColour(juce::Slider::textBoxBackgroundColourId, kBackground.brighter(0.06f));
    setColour(juce::Slider::textBoxOutlineColourId, kAccent.withAlpha(0.35f));

    setColour(juce::TextButton::buttonColourId, kBackground.brighter(0.08f));
    setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    setColour(juce::ToggleButton::textColourId, juce::Colours::white);

    setColour(juce::ListBox::backgroundColourId, kBackground.brighter(0.03f));
    setColour(juce::ListBox::textColourId, juce::Colours::white);
}

void LookAndFeel::drawRotarySlider(juce::Graphics& g,
                                   int x,
                                   int y,
                                   int width,
                                   int height,
                                   float sliderPos,
                                   float rotaryStartAngle,
                                   float rotaryEndAngle,
                                   juce::Slider&)
{
    const auto bounds = juce::Rectangle<float>(float(x), float(y), float(width), float(height)).reduced(6.0f);
    const auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    g.setColour(juce::Colours::white.withAlpha(0.10f));
    g.fillEllipse(bounds);

    g.setColour(kAccent.withAlpha(0.85f));
    juce::Path p;
    p.addPieSegment(bounds, rotaryStartAngle, angle, 0.75f);
    g.fillPath(p);

    g.setColour(kBackground.brighter(0.08f));
    g.fillEllipse(bounds.reduced(radius * 0.30f));

    juce::Path needle;
    needle.startNewSubPath(centre);
    needle.lineTo(centre.getX() + std::cos(angle) * radius * 0.85f, centre.getY() + std::sin(angle) * radius * 0.85f);
    g.setColour(juce::Colours::white.withAlpha(0.75f));
    g.strokePath(needle, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}
} // namespace mfpr

