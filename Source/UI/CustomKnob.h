#pragma once
#include <JuceHeader.h>
#include "CustomLookAndFeel.h"
namespace retrograde::ui
{
class CustomKnob final : public juce::Component
{
public:
    CustomKnob (juce::String title, juce::String suffix = {})
    {
        label.setText (title, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setColour (juce::Label::textColourId, Palette::text());
        label.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 100, 24);
        slider.setTextValueSuffix (suffix);
        addAndMakeVisible (label); addAndMakeVisible (slider);
    }
    juce::Slider& control() noexcept { return slider; }
    void resized() override
    {
        auto r = getLocalBounds(); label.setBounds (r.removeFromTop (24));
        const int side = juce::jmax (64, juce::jmin (r.getWidth() - 18, r.getHeight() - 28));
        slider.setBounds (r.withSizeKeepingCentre (side, side + 28));
    }
private:
    juce::Label label;
    juce::Slider slider;
};
}
