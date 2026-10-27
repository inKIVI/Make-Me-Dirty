#pragma once

#include <JuceHeader.h>
#include <array>
#include <cstdint>

namespace retrograde::dsp
{
class NoiseGenerator
{
public:
    enum class Type { hiss = 0, digital, staticNoise };

    void prepare (double sampleRate)
    {
        amount.reset (sampleRate, 0.04);
        amount.setCurrentAndTargetValue (0.0f);
        pink.fill (0.0f);
    }

    void setParameters (float percent, int typeIndex) noexcept
    {
        amount.setTargetValue (juce::jlimit (0.0f, 1.0f, percent * 0.01f));
        type = static_cast<Type> (juce::jlimit (0, 2, typeIndex));
    }

    void process (juce::AudioBuffer<float>& buffer) noexcept
    {
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float a = amount.getNextValue();
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                float n = white();
                if (type == Type::hiss)
                {
                    pink[(size_t) juce::jmin (ch, 1)] = 0.97f * pink[(size_t) juce::jmin (ch, 1)] + 0.03f * n;
                    n = n - pink[(size_t) juce::jmin (ch, 1)] * 0.65f;
                }
                else if (type == Type::digital)
                    n = std::round (n * 7.0f) / 7.0f;
                else if ((rng & 0x7ffu) != 0u)
                    n *= 0.12f;

                buffer.addSample (ch, i, n * a * 0.075f);
            }
        }
    }

private:
    float white() noexcept
    {
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
        return static_cast<float> (rng & 0xffffu) / 32767.5f - 1.0f;
    }
    juce::SmoothedValue<float> amount;
    std::array<float, 2> pink {};
    uint32_t rng { 0xa17c9e3du };
    Type type { Type::hiss };
};
}

