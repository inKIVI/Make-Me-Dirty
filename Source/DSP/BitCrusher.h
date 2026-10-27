#pragma once

#include <JuceHeader.h>
#include <array>
#include <cmath>
#include <cstdint>

namespace retrograde::dsp
{
class BitCrusher
{
public:
    enum class Character { clean = 0, triangularDither, asymmetric };

    void prepare (double sampleRate, int channels)
    {
        smoothBits.reset (sampleRate, 0.025);
        smoothBits.setCurrentAndTargetValue (16.0f);
        errors.fill (0.0f);
        numChannels = juce::jlimit (1, (int) errors.size(), channels);
    }

    void setParameters (float bits, int newCharacter) noexcept
    {
        smoothBits.setTargetValue (juce::jlimit (4.0f, 16.0f, bits));
        character = static_cast<Character> (juce::jlimit (0, 2, newCharacter));
    }

    void process (juce::AudioBuffer<float>& buffer) noexcept
    {
        const auto samples = buffer.getNumSamples();
        const auto channels = juce::jmin (numChannels, buffer.getNumChannels());
        for (int i = 0; i < samples; ++i)
        {
            const float bits = smoothBits.getNextValue();
            const float levels = std::pow (2.0f, bits - 1.0f);
            for (int ch = 0; ch < channels; ++ch)
            {
                float x = buffer.getSample (ch, i);
                if (character == Character::triangularDither)
                    x += (randomSigned() + randomSigned()) * (0.5f / levels);
                else if (character == Character::asymmetric)
                    x = x >= 0.0f ? x : x * 0.94f + errors[(size_t) ch] * 0.12f;

                const float q = std::round (juce::jlimit (-1.0f, 1.0f, x) * levels) / levels;
                errors[(size_t) ch] = x - q;
                buffer.setSample (ch, i, q);
            }
        }
    }

private:
    float randomSigned() noexcept
    {
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
        return static_cast<float> (rng & 0xffffu) / 32767.5f - 1.0f;
    }

    juce::SmoothedValue<float> smoothBits;
    std::array<float, 2> errors {};
    uint32_t rng { 0x91e10da5u };
    int numChannels { 2 };
    Character character { Character::clean };
};
}

