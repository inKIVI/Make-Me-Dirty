#pragma once

#include <JuceHeader.h>
#include <array>

namespace retrograde::dsp
{
class SampleRateReducer
{
public:
    enum class Interpolation { hold = 0, linear };

    void prepare (double newSampleRate, int channels)
    {
        sampleRate = newSampleRate;
        numChannels = juce::jlimit (1, 2, channels);
        phase = 1.0;
        held.fill (0.0f);
        previous.fill (0.0f);
        smoothRate.reset (sampleRate, 0.03);
        smoothRate.setCurrentAndTargetValue ((float) sampleRate);
    }

    void setParameters (float effectiveRate, int interpolationMode) noexcept
    {
        smoothRate.setTargetValue (juce::jlimit (1000.0f, (float) sampleRate, effectiveRate));
        interpolation = interpolationMode == 0 ? Interpolation::hold : Interpolation::linear;
    }

    void process (juce::AudioBuffer<float>& buffer) noexcept
    {
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const double step = smoothRate.getNextValue() / sampleRate;
            phase += step;
            if (phase >= 1.0)
            {
                phase -= 1.0;
                for (int ch = 0; ch < juce::jmin (numChannels, buffer.getNumChannels()); ++ch)
                {
                    previous[(size_t) ch] = held[(size_t) ch];
                    held[(size_t) ch] = buffer.getSample (ch, i);
                }
            }

            for (int ch = 0; ch < juce::jmin (numChannels, buffer.getNumChannels()); ++ch)
            {
                float y = held[(size_t) ch];
                if (interpolation == Interpolation::linear)
                    y = previous[(size_t) ch] + (held[(size_t) ch] - previous[(size_t) ch]) * (float) phase;
                buffer.setSample (ch, i, y);
            }
        }
    }

private:
    juce::SmoothedValue<float> smoothRate;
    std::array<float, 2> held {}, previous {};
    double sampleRate { 44100.0 }, phase { 1.0 };
    int numChannels { 2 };
    Interpolation interpolation { Interpolation::hold };
};
}

