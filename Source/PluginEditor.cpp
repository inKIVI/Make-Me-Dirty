#include "PluginEditor.h"

RetrogradeFXAudioProcessorEditor::RetrogradeFXAudioProcessorEditor (RetrogradeFXAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p), inputMeter (p.inputMeter, "IN"), outputMeter (p.outputMeter, "OUT")
{
    setLookAndFeel (&lookAndFeel);
    setResizable (true, true); setResizeLimits (900, 580, 1600, 980); setSize (1200, 760);

    title.setText ("MAKE ME DIRTY", juce::dontSendNotification);
    title.setColour (juce::Label::textColourId, retrograde::ui::Palette::text());
    title.setFont (juce::FontOptions (29.0f, juce::Font::bold));
    subtitle.setText ("RETRO AUDIO DEGRADATION", juce::dontSendNotification);
    subtitle.setColour (juce::Label::textColourId, retrograde::ui::Palette::dim());
    subtitle.setFont (juce::FontOptions (10.0f));
    credit.setText ("by inkivi", juce::dontSendNotification);
    credit.setColour (juce::Label::textColourId, retrograde::ui::Palette::rust().brighter (0.7f));
    credit.setFont (juce::FontOptions (11.0f));
    addAndMakeVisible (title); addAndMakeVisible (subtitle); addAndMakeVisible (credit);
    addAndMakeVisible (presetBox); addAndMakeVisible (previous); addAndMakeVisible (next);
    addAndMakeVisible (save); addAndMakeVisible (manage); addAndMakeVisible (bypass);

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        addAndMakeVisible (*knobs[i]);
        sliderAttachments.push_back (std::make_unique<SliderAttachment> (processor.parameters, knobIds[i], knobs[i]->control()));
        if (auto* parameter = processor.parameters.getParameter (knobIds[i]))
            knobs[i]->control().setDoubleClickReturnValue (true, parameter->convertFrom0to1 (parameter->getDefaultValue()));
    }
    mix.setSliderStyle (juce::Slider::LinearHorizontal);
    mix.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 86, 24);
    mix.setTextValueSuffix (" %");
    mixLabel.setText ("DRY / WET", juce::dontSendNotification);
    mixLabel.setJustificationType (juce::Justification::centred);
    mixLabel.setColour (juce::Label::textColourId, retrograde::ui::Palette::text());
    addAndMakeVisible (mixLabel); addAndMakeVisible (mix);
    mixAttachment = std::make_unique<SliderAttachment> (processor.parameters, "mix", mix);

    quantizer.addItemList ({ "Clean quantization", "Dither", "Asymmetric" }, 1);
    interpolation.addItemList ({ "Sample hold", "Linear interpolation" }, 1);
    distortionMode.addItemList ({ "Soft clip", "Hard clip", "Digital" }, 1);
    noiseType.addItemList ({ "Tape hiss", "Digital noise", "Static" }, 1);
    for (size_t i = 0; i < combos.size(); ++i)
    {
        addAndMakeVisible (*combos[i]);
        comboAttachments.push_back (std::make_unique<ComboAttachment> (processor.parameters, comboIds[i], *combos[i]));
    }
    addAndMakeVisible (inputMeter); addAndMakeVisible (outputMeter);

    bypassAttachment = std::make_unique<ButtonAttachment> (processor.parameters, "bypass", bypass);
    previous.onClick = [this] { processor.presetManager.loadPrevious(); rebuildPresetBox(); };
    next.onClick = [this] { processor.presetManager.loadNext(); rebuildPresetBox(); };
    save.onClick = [this] { showSaveDialog(); };
    manage.onClick = [this] { showManageMenu(); };
    presetBox.onChange = [this]
    {
        if (presetBox.getSelectedId() > 0) processor.presetManager.loadPreset (presetBox.getSelectedId() - 1);
    };
    processor.presetManager.onPresetListChanged = [safe = juce::Component::SafePointer<RetrogradeFXAudioProcessorEditor> (this)]
    {
        if (safe != nullptr) safe->rebuildPresetBox();
    };
    rebuildPresetBox();
    startTimerHz (30);
}

RetrogradeFXAudioProcessorEditor::~RetrogradeFXAudioProcessorEditor()
{
    processor.presetManager.onPresetListChanged = nullptr;
    setLookAndFeel (nullptr);
}

void RetrogradeFXAudioProcessorEditor::rebuildPresetBox()
{
    presetBox.clear (juce::dontSendNotification);
    for (int i = 0; i < processor.presetManager.getNumPresets(); ++i)
        presetBox.addItem (processor.presetManager.getPresetName (i), i + 1);
    const int index = processor.presetManager.getCurrentPresetIndex();
    if (index >= 0) presetBox.setSelectedId (index + 1, juce::dontSendNotification);
    else presetBox.setText (processor.presetManager.getCurrentPresetName(), juce::dontSendNotification);
}

void RetrogradeFXAudioProcessorEditor::promptForName (const juce::String& heading, const juce::String& initial,
                                                       std::function<void(const juce::String&)> completion)
{
    nameDialog = std::make_unique<juce::AlertWindow> (heading, "Enter a preset name", juce::MessageBoxIconType::NoIcon);
    nameDialog->addTextEditor ("name", initial, "NAME");
    nameDialog->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
    nameDialog->addButton ("CANCEL", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    auto safe = juce::Component::SafePointer<RetrogradeFXAudioProcessorEditor> (this);
    nameDialog->enterModalState (true, juce::ModalCallbackFunction::create ([safe, fn = std::move (completion)] (int result) mutable
    {
        if (safe == nullptr) return;
        if (result == 1 && safe->nameDialog != nullptr) fn (safe->nameDialog->getTextEditorContents ("name"));
        safe->nameDialog.reset();
    }), false);
}

void RetrogradeFXAudioProcessorEditor::showSaveDialog()
{
    promptForName ("SAVE USER PRESET", processor.presetManager.getCurrentPresetName(), [this] (const juce::String& name)
    { processor.presetManager.saveCustomPreset (name); rebuildPresetBox(); });
}

void RetrogradeFXAudioProcessorEditor::showManageMenu()
{
    juce::PopupMenu menu;
    const int presetIndex = processor.presetManager.getCurrentPresetIndex();
    const bool custom = presetIndex >= 0 && ! processor.presetManager.isFactoryPreset (presetIndex);
    menu.addItem (1, "Rename current preset", custom);
    menu.addItem (2, "Delete current preset", custom);
    menu.addSeparator(); menu.addItem (3, "Restore factory presets");
    menu.showMenuAsync ({}, [safe = juce::Component::SafePointer<RetrogradeFXAudioProcessorEditor> (this)] (int result)
    {
        if (safe == nullptr) return;
        if (result == 1)
            safe->promptForName ("RENAME USER PRESET", safe->processor.presetManager.getCurrentPresetName(), [safe] (const juce::String& name)
            { if (safe != nullptr) safe->processor.presetManager.renameCurrentPreset (name); });
        else if (result == 2) safe->processor.presetManager.deleteCurrentPreset();
        else if (result == 3) safe->processor.presetManager.restoreFactoryPresets();
    });
}

void RetrogradeFXAudioProcessorEditor::timerCallback() { inputMeter.repaint(); outputMeter.repaint(); }

void RetrogradeFXAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (retrograde::ui::Palette::background());
    auto drawPanel = [&g] (juce::Rectangle<int> area)
    {
        auto r = area.toFloat();
        g.setGradientFill (juce::ColourGradient (retrograde::ui::Palette::raised(), r.getX(), r.getY(),
                                                 retrograde::ui::Palette::panel(), r.getRight(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (retrograde::ui::Palette::edge()); g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.0f);
        g.setColour (retrograde::ui::Palette::rust().withAlpha (0.38f));
        g.fillRect (r.getX() + 3, r.getBottom() - 3, r.getWidth() - 6, 2.0f);
    };
    drawPanel (headerBounds);
    for (auto r : moduleBounds) drawPanel (r);
    for (auto r : bottomBounds) drawPanel (r);
}

void RetrogradeFXAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (16);
    headerBounds = area.removeFromTop (92); area.removeFromTop (12);
    auto header = headerBounds.reduced (18, 8);
    auto brand = header.removeFromLeft (juce::jmin (340, header.getWidth() / 3));
    title.setBounds (brand.removeFromTop (39)); subtitle.setBounds (brand.removeFromTop (19)); credit.setBounds (brand);
    bypass.setBounds (header.removeFromRight (86).reduced (3, 22));
    manage.setBounds (header.removeFromRight (42).reduced (3, 22));
    save.setBounds (header.removeFromRight (70).reduced (3, 22));
    next.setBounds (header.removeFromRight (40).reduced (3, 22));
    previous.setBounds (header.removeFromRight (40).reduced (3, 22));
    presetBox.setBounds (header.reduced (8, 22));

    const int bottomHeight = juce::jmax (145, area.getHeight() / 4);
    auto bottom = area.removeFromBottom (bottomHeight); area.removeFromBottom (12);
    const int gap = 10;
    for (int col = 0; col < 3; ++col)
    {
        const int left = bottom.getX() + col * bottom.getWidth() / 3;
        const int right = bottom.getX() + (col + 1) * bottom.getWidth() / 3;
        bottomBounds[(size_t) col] = { left + gap / 2, bottom.getY(), right - left - gap, bottom.getHeight() };
    }
    auto in = bottomBounds[0].reduced (12, 6);
    input.setBounds (in.removeFromLeft (in.getWidth() / 2)); inputMeter.setBounds (in.withSizeKeepingCentre (in.getWidth() - 12, 22));
    auto wet = bottomBounds[1].reduced (18, 16);
    mixLabel.setBounds (wet.removeFromTop (28)); mix.setBounds (wet);
    auto out = bottomBounds[2].reduced (12, 6);
    output.setBounds (out.removeFromLeft (out.getWidth() / 2)); outputMeter.setBounds (out.withSizeKeepingCentre (out.getWidth() - 12, 22));

    for (int row = 0; row < 2; ++row)
        for (int col = 0; col < 4; ++col)
        {
            const int index = row * 4 + col;
            const int left = area.getX() + col * area.getWidth() / 4;
            const int right = area.getX() + (col + 1) * area.getWidth() / 4;
            const int top = area.getY() + row * area.getHeight() / 2;
            const int height = area.getHeight() / 2 - gap;
            moduleBounds[(size_t) index] = { left + gap / 2, top, right - left - gap, height };
            auto inner = moduleBounds[(size_t) index].reduced (8, 6);
            if (index == 0 || index == 1 || index == 3 || index == 7)
            {
                auto* mode = index == 0 ? &quantizer : index == 1 ? &interpolation : index == 3 ? &distortionMode : &noiseType;
                mode->setBounds (inner.removeFromBottom (28).reduced (8, 2));
            }
            knobs[(size_t) index]->setBounds (inner);
        }
    repaint();
}

