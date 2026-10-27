#pragma once

#include <JuceHeader.h>
#include <array>
#include <cmath>
#include <vector>

namespace retrograde::dsp
{
// A low-latency two-head granular delay pitch shifter. Two Hann-windowed read
// heads crossfade half a grain apart while the write head continues in real
// time, so pitch changes do not alter stream duration.
class PitchShifter
{
public:
    void prepare (double newSampleRate, int maximumBlockSize, int channels)
    {
        sampleRate = newSampleRate;
        numChannels = juce::jlimit (1, 2, channels);
        grainSize = juce::jlimit (512, 4096, (int) std::round (sampleRate * 0.046));
        bufferSize = juce::nextPowerOfTwo (grainSize * 4 + maximumBlockSize);
        delay.assign ((size_t) numChannels, std::vector<float> ((size_t) bufferSize, 0.0f));
        writePosition = 0;
        phase = 0.0;
        ratio.reset (sampleRate, 0.04);
        ratio.setCurrentAndTargetValue (1.0f);
    }

    void reset() noexcept
    {
        for (auto& channel : delay) std::fill (channel.begin(), channel.end(), 0.0f);
        writePosition = 0;
        phase = 0.0;
    }

    void setSemitones (float semitones) noexcept
    {
        ratio.setTargetValue (std::pow (2.0f, juce::jlimit (-12.0f, 12.0f, semitones) / 12.0f));
    }

    int getLatencySamples() const noexcept { return 64 + grainSize / 2; }

    void process (juce::AudioBuffer<float>& buffer) noexcept
    {
        if (delay.empty()) return;

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const double pitchRatio = ratio.getNextValue();
            // Delay trajectory slope is 1-ratio. Wrapping the trajectory at the
            // grain boundary creates the pitch change; the second head masks it.
            phase += (1.0 - pitchRatio) / (double) grainSize;
            phase -= std::floor (phase);
            const double phaseB = std::fmod (phase + 0.5, 1.0);
            const float windowA = hann (phase);
            const float windowB = hann (phaseB);
            const float norm = 1.0f / juce::jmax (0.001f, windowA + windowB);

            for (int ch = 0; ch < juce::jmin (numChannels, buffer.getNumChannels()); ++ch)
            {
                auto& line = delay[(size_t) ch];
                line[(size_t) writePosition] = buffer.getSample (ch, i);
                const double delayA = 64.0 + phase * grainSize;
                const double delayB = 64.0 + phaseB * grainSize;
                const float y = (readInterpolated (line, delayA) * windowA
                               + readInterpolated (line, delayB) * windowB) * norm;
                buffer.setSample (ch, i, y);
            }
            writePosition = (writePosition + 1) & (bufferSize - 1);
        }
    }

private:
    static float hann (double p) noexcept
    {
        return 0.5f - 0.5f * std::cos ((float) (juce::MathConstants<double>::twoPi * p));
    }

    float readInterpolated (const std::vector<float>& line, double delaySamples) const noexcept
    {
        double position = (double) writePosition - delaySamples;
        while (position < 0.0) position += bufferSize;
        const int a = (int) position & (bufferSize - 1);
        const int b = (a + 1) & (bufferSize - 1);
        const float frac = (float) (position - std::floor (position));
        return line[(size_t) a] + frac * (line[(size_t) b] - line[(size_t) a]);
    }

    std::vector<std::vector<float>> delay;
    juce::SmoothedValue<float> ratio;
    double sampleRate { 44100.0 }, phase { 0.0 };
    int numChannels { 2 }, grainSize { 2048 }, bufferSize { 8192 }, writePosition { 0 };
};
}

