#include "PluginEditor.h"
#include "PluginProcessor.h"

namespace mfpr
{
static void addItems(juce::ComboBox& cb, const std::array<juce::String, 32>& items)
{
    int id = 1;
    for (const auto& s : items)
        cb.addItem(s, id++);
}

static void addItems(juce::ComboBox& cb, const std::array<juce::String, 12>& items)
{
    int id = 1;
    for (const auto& s : items)
        cb.addItem(s, id++);
}

static void addItems(juce::ComboBox& cb, const std::array<juce::String, 4>& items)
{
    int id = 1;
    for (const auto& s : items)
        cb.addItem(s, id++);
}

static void addItems(juce::ComboBox& cb, const std::array<juce::String, 3>& items)
{
    int id = 1;
    for (const auto& s : items)
        cb.addItem(s, id++);
}

static void addItems(juce::ComboBox& cb, const std::array<juce::String, 2>& items)
{
    int id = 1;
    for (const auto& s : items)
        cb.addItem(s, id++);
}

MelodyForgeProAudioEditor::MelodyForgeProAudioEditor(MelodyForgeProAudioProcessor& p)
    : AudioProcessorEditor(&p)
    , processor(p)
    , pianoRoll(p)
    , samplerSlots(p)
{
    setLookAndFeel(&lookAndFeel);

    setSize(mfpr::kEditorWidth, mfpr::kEditorHeight);
    setResizable(false, false);

    configureComboBox(genreBox);
    configureComboBox(keyBox);
    configureComboBox(modeBox);
    configureComboBox(lengthBox);
    configureComboBox(typeBox);

    addItems(genreBox, mfpr::kGenres);
    addItems(keyBox, mfpr::kKeys);
    addItems(modeBox, mfpr::kModes);
    addItems(lengthBox, mfpr::kLengths);
    addItems(typeBox, mfpr::kTypes);

    addAndMakeVisible(genreBox);
    addAndMakeVisible(keyBox);
    addAndMakeVisible(modeBox);
    addAndMakeVisible(lengthBox);
    addAndMakeVisible(typeBox);

    auto& apvts = processor.getAPVTS();
    genreAttach = std::make_unique<APVTS::ComboBoxAttachment>(apvts, "genre", genreBox);
    keyAttach = std::make_unique<APVTS::ComboBoxAttachment>(apvts, "key", keyBox);
    modeAttach = std::make_unique<APVTS::ComboBoxAttachment>(apvts, "mode", modeBox);
    lengthAttach = std::make_unique<APVTS::ComboBoxAttachment>(apvts, "length", lengthBox);
    typeAttach = std::make_unique<APVTS::ComboBoxAttachment>(apvts, "type", typeBox);

    // Left action panel
    addAndMakeVisible(leftActions);
    leftActions.addAndMakeVisible(generateButton);
    leftActions.addAndMakeVisible(exportButton);
    leftActions.addAndMakeVisible(animationToggle);

    generateButton.setColour(juce::TextButton::buttonColourId, mfpr::kAccent.withAlpha(0.85f));
    generateButton.setColour(juce::TextButton::textColourOffId, juce::Colours::black);
    generateButton.onClick = [this] { processor.triggerGenerateFromUI(); };

    exportButton.onClick = [this] { doExportDrag(); };

    animationToggle.onClick = [this] { updateParticles(); };

    // Control 7/8/9/10-13
    addAndMakeVisible(pianoRoll);
    addAndMakeVisible(presetList);
    addAndMakeVisible(samplerSlots);

    addAndMakeVisible(macroArea);
    macroArea.addAndMakeVisible(macro1);
    macroArea.addAndMakeVisible(macro2);
    macroArea.addAndMakeVisible(macro3);
    macroArea.addAndMakeVisible(macro4);

    configureMacroSlider(macro1, "Macro 1");
    configureMacroSlider(macro2, "Macro 2");
    configureMacroSlider(macro3, "Macro 3");
    configureMacroSlider(macro4, "Macro 4");

    macro1Attach = std::make_unique<APVTS::SliderAttachment>(apvts, "macro01", macro1);
    macro2Attach = std::make_unique<APVTS::SliderAttachment>(apvts, "macro02", macro2);
    macro3Attach = std::make_unique<APVTS::SliderAttachment>(apvts, "macro03", macro3);
    macro4Attach = std::make_unique<APVTS::SliderAttachment>(apvts, "macro04", macro4);

    // Particles behind everything
    addAndMakeVisible(particles);
    particles.toBack();
    particles.setEnabled(false);

    presetList.setRowHeight(22);
    presetList.updateContent();
    presetList.selectRow(0);

    setWantsKeyboardFocus(true);
    startTimerHz(10);
}

MelodyForgeProAudioEditor::~MelodyForgeProAudioEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void MelodyForgeProAudioEditor::configureComboBox(juce::ComboBox& cb)
{
    cb.setJustificationType(juce::Justification::centredLeft);
}

void MelodyForgeProAudioEditor::configureMacroSlider(juce::Slider& s, const juce::String& name)
{
    s.setName(name);
    s.setRange(0.0, 127.0, 1.0);
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
}

void MelodyForgeProAudioEditor::paint(juce::Graphics& g)
{
    g.fillAll(mfpr::kBackground);

    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.setFont(juce::Font(18.0f, juce::Font::bold));
    g.drawText(mfpr::kPluginDisplayName, 12, 6, getWidth() - 24, 22, juce::Justification::centredLeft);

    g.setColour(juce::Colours::white.withAlpha(0.60f));
    g.setFont(juce::Font(12.5f));
    g.drawText("Presets: right-click for user slots (save/load 1-5). Export creates a .mid temp file and starts external drag.",
               12,
               28,
               getWidth() - 24,
               18,
               juce::Justification::centredLeft);
}

void MelodyForgeProAudioEditor::resized()
{
    particles.setBounds(getLocalBounds());

    juce::Grid grid;
    grid.templateAreas = {
        "genre key mode length type",
        "actions roll roll roll presets",
        "sampler macros macros macros macros",
    };
    grid.templateRows = { juce::Grid::TrackInfo(juce::Grid::Px(60.0f)),
                          juce::Grid::TrackInfo(juce::Grid::Px(360.0f)),
                          juce::Grid::TrackInfo(juce::Grid::Fr(1)) };

    grid.templateColumns = { juce::Grid::TrackInfo(juce::Grid::Px(220.0f)),
                             juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                             juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                             juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                             juce::Grid::TrackInfo(juce::Grid::Fr(1)) };

    grid.rowGap = juce::Grid::Px(8.0f);
    grid.columnGap = juce::Grid::Px(8.0f);

    grid.items = {
        juce::GridItem(genreBox).withArea("genre"),
        juce::GridItem(keyBox).withArea("key"),
        juce::GridItem(modeBox).withArea("mode"),
        juce::GridItem(lengthBox).withArea("length"),
        juce::GridItem(typeBox).withArea("type"),

        juce::GridItem(leftActions).withArea("actions"),
        juce::GridItem(pianoRoll).withArea("roll"),
        juce::GridItem(presetList).withArea("presets"),

        juce::GridItem(samplerSlots).withArea("sampler"),
        juce::GridItem(macroArea).withArea("macros")
    };

    auto area = getLocalBounds().reduced(12, 52);
    grid.performLayout(area);

    // Left actions layout with exact 200x50 generate button.
    auto left = leftActions.getLocalBounds().reduced(10);
    generateButton.setBounds(left.removeFromTop(50).withSizeKeepingCentre(200, 50));
    left.removeFromTop(10);
    exportButton.setBounds(left.removeFromTop(32));
    left.removeFromTop(10);
    animationToggle.setBounds(left.removeFromTop(24));

    // Macro area layout
    auto macroBounds = macroArea.getLocalBounds().reduced(8);
    juce::Grid mg;
    mg.templateRows = { juce::Grid::TrackInfo(juce::Grid::Fr(1)) };
    mg.templateColumns = { juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                           juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                           juce::Grid::TrackInfo(juce::Grid::Fr(1)),
                           juce::Grid::TrackInfo(juce::Grid::Fr(1)) };
    mg.columnGap = juce::Grid::Px(8.0f);
    mg.items = { juce::GridItem(macro1), juce::GridItem(macro2), juce::GridItem(macro3), juce::GridItem(macro4) };
    mg.performLayout(macroBounds);
}

int MelodyForgeProAudioEditor::getNumRows()
{
    return processor.getPresetManager().getNumPresets();
}

void MelodyForgeProAudioEditor::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool selected)
{
    if (selected)
    {
        g.setColour(mfpr::kAccent.withAlpha(0.18f));
        g.fillRect(0, 0, width, height);
    }

    g.setColour(juce::Colours::white.withAlpha(selected ? 0.95f : 0.75f));
    g.setFont(juce::Font(13.0f));
    g.drawText(processor.getPresetManager().getPresetName(rowNumber), 8, 0, width - 10, height, juce::Justification::centredLeft);
}

void MelodyForgeProAudioEditor::selectedRowsChanged(int lastRowSelected)
{
    if (lastRowSelected >= 0)
        processor.getPresetManager().applyBuiltinPreset(lastRowSelected);
}

void MelodyForgeProAudioEditor::listBoxItemClicked(int row, const juce::MouseEvent& e)
{
    if (!e.mods.isRightButtonDown())
        return;

    juce::PopupMenu m;
    m.addItem(1, "Load Built-in Preset");
    m.addSeparator();

    juce::PopupMenu loadSlots;
    juce::PopupMenu saveSlots;
    for (int i = 0; i < 5; ++i)
    {
        loadSlots.addItem(10 + i, juce::String::formatted("Load User Slot %d", i + 1));
        saveSlots.addItem(20 + i, juce::String::formatted("Save to User Slot %d", i + 1));
    }
    m.addSubMenu("User Slots", loadSlots, true);
    m.addSubMenu("Save Slots", saveSlots, true);

    m.showMenuAsync(juce::PopupMenu::Options(), [this, row](int result)
    {
        if (result == 1 && row >= 0)
            processor.getPresetManager().applyBuiltinPreset(row);
        else if (result >= 10 && result < 15)
            processor.getPresetManager().loadUserSlot(result - 10);
        else if (result >= 20 && result < 25)
            processor.getPresetManager().saveUserSlot(result - 20);
    });
}

bool MelodyForgeProAudioEditor::keyPressed(const juce::KeyPress& key)
{
    const auto c = key.getTextCharacter();
    if (c >= '1' && c <= '5')
    {
        const int slot = int(c - '1');
        if (key.getModifiers().isShiftDown())
            processor.getPresetManager().saveUserSlot(slot);
        else
            processor.getPresetManager().loadUserSlot(slot);
        return true;
    }
    return false;
}

void MelodyForgeProAudioEditor::doExportDrag()
{
    auto file = juce::File::getSpecialLocation(juce::File::tempDirectory)
                    .getNonexistentChildFile("MelodyForgePro_Export", ".mid");

    if (processor.exportCurrentPatternToFile(file))
    {
        juce::StringArray files;
        files.add(file.getFullPathName());
        performExternalDragDropOfFiles(files, false, nullptr);
    }
}

void MelodyForgeProAudioEditor::updateParticles()
{
    particles.setEnabled(animationToggle.getToggleState());
}

void MelodyForgeProAudioEditor::timerCallback()
{
    // Keep particles in sync if host resets UI state.
    updateParticles();
}
} // namespace mfpr
