#pragma once
#include <JuceHeader.h>

namespace retrograde::ui
{
struct Palette
{
    static juce::Colour background() { return juce::Colour (0xff171614); }
    static juce::Colour panel() { return juce::Colour (0xff25221e); }
    static juce::Colour raised() { return juce::Colour (0xff38332d); }
    static juce::Colour edge() { return juce::Colour (0xff514a40); }
    static juce::Colour rust() { return juce::Colour (0xff754631); }
    static juce::Colour text() { return juce::Colour (0xffc7b9a4); }
    static juce::Colour dim() { return juce::Colour (0xff8c8274); }
    static juce::Colour phosphor() { return juce::Colour (0xffa5b992); }
    static juce::Colour active() { return juce::Colour (0xffc16a4b); }
};
class CustomLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId, Palette::phosphor());
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff151a16));
        setColour (juce::Slider::textBoxOutlineColourId, Palette::edge());
        setColour (juce::ComboBox::backgroundColourId, Palette::raised());
        setColour (juce::ComboBox::outlineColourId, Palette::edge());
        setColour (juce::ComboBox::textColourId, Palette::text());
        setColour (juce::PopupMenu::backgroundColourId, Palette::panel());
        setColour (juce::PopupMenu::textColourId, Palette::text());
        setColour (juce::TextButton::textColourOffId, Palette::text());
    }
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float position,
                           float start, float end, juce::Slider&) override
    {
        const float d = (float) juce::jmin (w, h) - 14.0f;
        auto circle = juce::Rectangle<float> (d, d).withCentre ({ x + w * 0.5f, y + h * 0.5f });
        const auto c = circle.getCentre();
        g.setColour (juce::Colours::black.withAlpha (0.4f)); g.fillEllipse (circle.translated (0, 4).expanded (3));
        g.setColour (Palette::rust().withAlpha (0.5f)); g.fillEllipse (circle.expanded (4));
        g.setColour (Palette::edge()); g.fillEllipse (circle.expanded (2));
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff565047), circle.getX(), circle.getY(),
                                                 juce::Colour (0xff24211e), circle.getRight(), circle.getBottom(), false));
        g.fillEllipse (circle);
        g.setColour (Palette::text().withAlpha (0.2f)); g.drawEllipse (circle.reduced (3), 1.0f);
        const float angle = start + position * (end - start);
        juce::Path mark; mark.addRoundedRectangle (-2.0f, -d * 0.40f, 4.0f, d * 0.18f, 2.0f);
        mark.applyTransform (juce::AffineTransform::rotation (angle).translated (c.x, c.y));
        g.setColour (Palette::phosphor()); g.fillPath (mark);
    }
    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float, float, const juce::Slider::SliderStyle style, juce::Slider&) override
    {
        if (style != juce::Slider::LinearHorizontal) return;
        auto track = juce::Rectangle<float> ((float) x + 12, y + h * 0.5f - 4, (float) w - 24, 8);
        g.setColour (juce::Colour (0xff121512)); g.fillRoundedRectangle (track, 3);
        g.setColour (Palette::edge()); g.drawRoundedRectangle (track, 3, 1);
        g.setColour (Palette::active()); g.fillRoundedRectangle (track.withWidth (juce::jmax (0.0f, pos - track.getX())), 3);
        g.setColour (Palette::text()); g.fillRoundedRectangle (pos - 6, track.getCentreY() - 15, 12, 30, 2);
    }
    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hover, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (down ? Palette::rust() : hover ? Palette::raised().brighter (0.1f) : Palette::raised());
        g.fillRoundedRectangle (r, 3); g.setColour (Palette::edge()); g.drawRoundedRectangle (r, 3, 1);
    }
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool hover, bool down) override
    {
        drawButtonBackground (g, b, {}, hover, down);
        g.setColour (b.getToggleState() ? Palette::active() : Palette::text());
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawFittedText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred, 1);
    }
};
}
