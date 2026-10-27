#pragma once

#include <JuceHeader.h>
#include "CustomLookAndFeel.h"
#include <atomic>

namespace retrograde::ui
{
class LevelMeter final : public juce::Component
{
public:
    LevelMeter (std::atomic<float>& source, juce::String text) : level (source), name (std::move (text)) {}
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (Palette::background()); g.fillRect (r);
        const float normal = juce::jlimit (0.0f, 1.0f, juce::jmap (level.load(), -60.0f, 0.0f, 0.0f, 1.0f));
        auto fill = r.reduced (2.0f); fill.setWidth (fill.getWidth() * normal);
        g.setColour (normal > 0.92f ? Palette::active() : Palette::phosphor()); g.fillRect (fill);
        g.setColour (Palette::edge()); g.drawRect (r, 1.0f);
        g.setColour (Palette::dim()); g.setFont (juce::FontOptions (10.0f));
        g.drawText (name, r.reduced (5, 0), juce::Justification::centredLeft);
    }
private:
    std::atomic<float>& level;
    juce::String name;
};
}

