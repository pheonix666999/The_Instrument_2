#include "PianoRollComponent.h"
#include "PluginProcessor.h"
#include <algorithm>

namespace mfpr
{
static constexpr int minNote = 36;
static constexpr int maxNote = 84;

static int positiveModulo(int a, int b)
{
    int result = a % b;
    return result >= 0 ? result : result + b;
}

static bool isInScale(int midiNote, int keyIndex, int modeIndex)
{
    static constexpr std::array<int, 7> major = { 0, 2, 4, 5, 7, 9, 11 };
    static constexpr std::array<int, 7> minor = { 0, 2, 3, 5, 7, 8, 10 };
    const auto pc = positiveModulo((midiNote % 12) - keyIndex, 12);
    const auto& scale = (modeIndex != 0) ? minor : major;
    return std::find(scale.begin(), scale.end(), pc) != scale.end();
}

PianoRollComponent::PianoRollComponent(MelodyForgeProAudioProcessor& p) : processor(p)
{
    startTimerHz(20);
}

PianoRollComponent::~PianoRollComponent()
{
    stopTimer();
}

int PianoRollComponent::yToMidiNote(int y) const
{
    const auto h = juce::jmax(1, getHeight());
    const auto norm = juce::jlimit(0.0, 1.0, double(y) / double(h));
    const auto note = maxNote - int(norm * double(maxNote - minNote + 1));
    return juce::jlimit(minNote, maxNote, note);
}

int PianoRollComponent::xToStep(int x) const
{
    const auto w = juce::jmax(1, getWidth());
    const auto len = pattern != nullptr ? pattern->lengthSteps : 64;
    const auto norm = juce::jlimit(0.0, 1.0, double(x) / double(w));
    const auto step = int(norm * double(len));
    return juce::jlimit(0, juce::jmax(0, len - 1), step);
}

juce::Rectangle<float> PianoRollComponent::noteToRect(const MidiNote& n, int minN, int maxN) const
{
    const auto len = juce::jmax(1, pattern != nullptr ? pattern->lengthSteps : 64);
    const auto w = float(getWidth());
    const auto h = float(getHeight());
    const auto noteCount = float(maxN - minN + 1);

    const auto x = (float(n.startStep) / float(len)) * w;
    const auto ww = (float(juce::jmax(1, n.lengthSteps)) / float(len)) * w;

    const auto y = (float(maxN - n.noteNumber) / noteCount) * h;
    const auto hh = h / noteCount;
    return { x, y, ww, hh };
}

void PianoRollComponent::timerCallback()
{
    pattern = processor.getCurrentPattern();
    keyIndex = processor.getKeyIndex();
    modeIndex = processor.getModeIndex();
    repaint();
}

void PianoRollComponent::mouseDown(const juce::MouseEvent& e)
{
    if (pattern == nullptr)
        return;

    auto edited = *pattern;

    const int note = yToMidiNote(e.y);
    const int step = xToStep(e.x);

    const int desiredTrack = processor.getTypeIndex() == int(GeneratorType::hybrid) ? 4 : 0;

    auto it = std::find_if(edited.notes.begin(), edited.notes.end(), [&](const MidiNote& n)
                           { return n.noteNumber == note && n.startStep == step && n.track == desiredTrack; });

    if (it != edited.notes.end())
        edited.notes.erase(it);
    else
        edited.notes.push_back(MidiNote{ note, step, 1, 100, 1, desiredTrack });

    processor.setEditedPattern(std::move(edited));
}

void PianoRollComponent::paint(juce::Graphics& g)
{
    g.fillAll(kBackground.brighter(0.02f));

    const auto b = getLocalBounds().toFloat();

    // Scale highlighting rows
    const int numNotes = maxNote - minNote + 1;
    const float rowH = b.getHeight() / float(numNotes);
    for (int i = 0; i < numNotes; ++i)
    {
        const int note = maxNote - i;
        if (isInScale(note, keyIndex, modeIndex))
        {
            g.setColour(kAccent.withAlpha(0.04f));
            g.fillRect(0.0f, rowH * float(i), b.getWidth(), rowH);
        }
    }

    // Grid
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    for (int i = 0; i <= numNotes; ++i)
        g.drawHorizontalLine(int(rowH * float(i)), 0.0f, b.getWidth());

    const int steps = pattern != nullptr ? pattern->lengthSteps : 64;
    for (int s = 0; s <= steps; ++s)
    {
        const float x = (float(s) / float(steps)) * b.getWidth();
        const float alpha = (s % 16 == 0) ? 0.18f : ((s % 4 == 0) ? 0.10f : 0.05f);
        g.setColour(juce::Colours::white.withAlpha(alpha));
        g.drawVerticalLine(int(x), 0.0f, b.getHeight());
    }

    // Notes
    if (pattern != nullptr)
    {
        for (const auto& n : pattern->notes)
        {
            if (n.noteNumber < minNote || n.noteNumber > maxNote)
                continue;

            const auto r = noteToRect(n, minNote, maxNote).reduced(0.5f, 0.5f);
            g.setColour(kAccent.withAlpha(0.70f));
            g.fillRoundedRectangle(r, 2.0f);

            g.setColour(kBackground.withAlpha(0.35f));
            g.drawRoundedRectangle(r, 2.0f, 1.0f);
        }
    }

    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.setFont(juce::Font(13.0f, juce::Font::bold));
    g.drawText("Piano Roll (click to toggle notes)", getLocalBounds().reduced(8).removeFromTop(18), juce::Justification::left);
}
} // namespace mfpr

