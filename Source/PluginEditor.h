#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/CustomLookAndFeel.h"
#include "UI/CustomKnob.h"
#include "UI/LevelMeter.h"
#include <array>
#include <memory>

class RetrogradeFXAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit RetrogradeFXAudioProcessorEditor (RetrogradeFXAudioProcessor&);
    ~RetrogradeFXAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    void timerCallback() override;
    void rebuildPresetBox();
    void showSaveDialog();
    void showManageMenu();
    void promptForName (const juce::String&, const juce::String&, std::function<void(const juce::String&)>);

    RetrogradeFXAudioProcessor& processor;
    retrograde::ui::CustomLookAndFeel lookAndFeel;
    juce::Label title, subtitle, credit, mixLabel;
    juce::ComboBox presetBox;
    juce::TextButton previous { "<" }, next { ">" }, save { "SAVE" }, manage { "..." };
    juce::ToggleButton bypass { "BYPASS" };

    retrograde::ui::CustomKnob bitDepth { "BITCRUSHER", " bit" }, sampleRate { "SAMPLE RATE", " Hz" }, pitch { "PITCH", " st" };
    retrograde::ui::CustomKnob drive { "DISTORTION", " dB" }, lowCut { "LOW CUT", " Hz" }, highCut { "HIGH CUT", " Hz" };
    retrograde::ui::CustomKnob midTone { "MID TONE", " dB" }, noise { "NOISE / TEXTURE", " %" };
    retrograde::ui::CustomKnob input { "INPUT", " dB" }, output { "OUTPUT", " dB" };
    juce::Slider mix;
    std::array<retrograde::ui::CustomKnob*, 10> knobs { &bitDepth,&sampleRate,&pitch,&drive,&lowCut,&highCut,&midTone,&noise,&input,&output };
    std::array<const char*, 10> knobIds { "bitDepth","sampleRate","pitch","drive","lowCut","highCut","midTone","noise","input","output" };
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;

    juce::ComboBox quantizer, interpolation, distortionMode, noiseType;
    std::array<juce::ComboBox*, 4> combos { &quantizer,&interpolation,&distortionMode,&noiseType };
    std::array<const char*, 4> comboIds { "bitCharacter","interpolation","distortionMode","noiseType" };
    std::vector<std::unique_ptr<ComboAttachment>> comboAttachments;
    std::unique_ptr<ButtonAttachment> bypassAttachment;
    std::unique_ptr<SliderAttachment> mixAttachment;
    retrograde::ui::LevelMeter inputMeter, outputMeter;
    std::unique_ptr<juce::AlertWindow> nameDialog;
    juce::Rectangle<int> headerBounds;
    std::array<juce::Rectangle<int>, 8> moduleBounds;
    std::array<juce::Rectangle<int>, 3> bottomBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RetrogradeFXAudioProcessorEditor)
};

