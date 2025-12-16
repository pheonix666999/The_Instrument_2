#include "SamplerSlotsComponent.h"
#include "PluginProcessor.h"

namespace mfpr
{
SamplerSlotsComponent::SamplerSlotsComponent(MelodyForgeProAudioProcessor& p) : processor(p)
{
    // Mouse move events are received automatically when mouseMove() is overridden
}

bool SamplerSlotsComponent::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& f : files)
        if (f.endsWithIgnoreCase(".mid") || f.endsWithIgnoreCase(".midi"))
            return true;
    return false;
}

void SamplerSlotsComponent::filesDropped(const juce::StringArray& files, int x, int y)
{
    if (files.isEmpty())
        return;

    const auto slot = hoveredSlot >= 0 ? hoveredSlot : slotAt(x, y);
    if (slot < 0)
        return;

    const juce::File file(files[0]);
    processor.importMidiToSamplerSlot(slot, file);
    repaint();
}

int SamplerSlotsComponent::slotAt(int x, int y) const
{
    for (int i = 0; i < 4; ++i)
        if (getSlotBounds(i).contains(x, y))
            return i;
    return -1;
}

juce::Rectangle<int> SamplerSlotsComponent::getSlotBounds(int slot) const
{
    auto r = getLocalBounds().reduced(6);
    const int w = r.getWidth() / 2;
    const int h = r.getHeight() / 2;

    const int col = slot % 2;
    const int row = slot / 2;
    return juce::Rectangle<int>(r.getX() + col * w, r.getY() + row * h, w, h).reduced(4);
}

void SamplerSlotsComponent::mouseMove(const juce::MouseEvent& e)
{
    const auto s = slotAt(e.x, e.y);
    if (s != hoveredSlot)
    {
        hoveredSlot = s;
        repaint();
    }
}

void SamplerSlotsComponent::mouseExit(const juce::MouseEvent&)
{
    hoveredSlot = -1;
    repaint();
}

void SamplerSlotsComponent::paint(juce::Graphics& g)
{
    g.setColour(kBackground.brighter(0.04f));
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(2.0f), 8.0f);

    g.setColour(juce::Colours::white.withAlpha(0.25f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(2.0f), 8.0f, 1.0f);

    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.drawText("Sampler Slots (Drop .mid) — Notes 60-63", getLocalBounds().reduced(10).removeFromTop(20), juce::Justification::left);

    for (int i = 0; i < 4; ++i)
    {
        auto r = getSlotBounds(i);
        const bool hot = (i == hoveredSlot);

        g.setColour(hot ? kAccent.withAlpha(0.15f) : kBackground.brighter(0.08f));
        g.fillRoundedRectangle(r.toFloat(), 6.0f);

        g.setColour(hot ? kAccent.withAlpha(0.75f) : juce::Colours::white.withAlpha(0.18f));
        g.drawRoundedRectangle(r.toFloat(), 6.0f, 1.0f);

        const auto info = processor.getSamplerSlotInfo(i);
        g.setColour(juce::Colours::white.withAlpha(0.85f));
        g.setFont(juce::Font(13.0f, juce::Font::bold));
        g.drawText(juce::String::formatted("Slot %d (Note %d)", i + 1, 60 + i), r.removeFromTop(18), juce::Justification::centredLeft);

        g.setColour(juce::Colours::white.withAlpha(0.65f));
        g.setFont(juce::Font(12.0f));
        g.drawFittedText(info.label, r.reduced(2), juce::Justification::centredLeft, 2);

        if (info.matchScore > 0.0)
        {
            g.setColour(kAccent.withAlpha(0.85f));
            g.drawText(juce::String::formatted("AI match: %.0f%%", info.matchScore * 100.0),
                       r.removeFromBottom(16),
                       juce::Justification::centredLeft);
        }
    }
}
} // namespace mfpr

