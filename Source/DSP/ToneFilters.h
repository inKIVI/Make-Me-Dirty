#pragma once

#include <JuceHeader.h>
#include <array>
#include <cmath>

namespace retrograde::dsp
{
class ToneFilters
{
public:
    void prepare (double newSampleRate, int channels)
    {
        sampleRate = newSampleRate;
        numChannels = juce::jlimit (1, 2, channels);
        reset();
        highPass.reset (sampleRate, 0.025);
        lowPass.reset (sampleRate, 0.025);
        midGain.reset (sampleRate, 0.025);
        highPass.setCurrentAndTargetValue (20.0f);
        lowPass.setCurrentAndTargetValue (20000.0f);
        midGain.setCurrentAndTargetValue (1.0f);
    }

    void reset() noexcept
    {
        hpX1.fill (0.0f); hpY1.fill (0.0f); lpY1.fill (0.0f);
        midLow.fill (0.0f); midBand.fill (0.0f);
    }

    void setParameters (float hpHz, float lpHz, float midDb) noexcept
    {
        highPass.setTargetValue (juce::jlimit (20.0f, 2000.0f, hpHz));
        lowPass.setTargetValue (juce::jlimit (1500.0f, 20000.0f, lpHz));
        midGain.setTargetValue (juce::Decibels::decibelsToGain (juce::jlimit (-12.0f, 12.0f, midDb)));
    }

    void process (juce::AudioBuffer<float>& buffer) noexcept
    {
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float hp = highPass.getNextValue();
            const float lp = juce::jmax (lowPass.getNextValue(), hp * 1.25f);
            const float gain = midGain.getNextValue();
            const float hpA = std::exp (-juce::MathConstants<float>::twoPi * hp / (float) sampleRate);
            const float lpA = std::exp (-juce::MathConstants<float>::twoPi * lp / (float) sampleRate);
            const float midA = std::exp (-juce::MathConstants<float>::twoPi * 1200.0f / (float) sampleRate);

            for (int ch = 0; ch < juce::jmin (numChannels, buffer.getNumChannels()); ++ch)
            {
                const float x = buffer.getSample (ch, i);
                const float highPassed = hpA * (hpY1[(size_t) ch] + x - hpX1[(size_t) ch]);
                hpX1[(size_t) ch] = x; hpY1[(size_t) ch] = highPassed;
                const float lowPassed = (1.0f - lpA) * highPassed + lpA * lpY1[(size_t) ch];
                lpY1[(size_t) ch] = lowPassed;

                midLow[(size_t) ch] = (1.0f - midA) * lowPassed + midA * midLow[(size_t) ch];
                const float band = lowPassed - midLow[(size_t) ch];
                midBand[(size_t) ch] += 0.12f * (band - midBand[(size_t) ch]);
                buffer.setSample (ch, i, lowPassed + midBand[(size_t) ch] * (gain - 1.0f));
            }
        }
    }

private:
    juce::SmoothedValue<float> highPass, lowPass, midGain;
    std::array<float, 2> hpX1 {}, hpY1 {}, lpY1 {}, midLow {}, midBand {};
    double sampleRate { 44100.0 };
    int numChannels { 2 };
};
}

