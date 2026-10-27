#pragma once

#include <JuceHeader.h>
#include <cmath>

namespace retrograde::dsp
{
class Distortion
{
public:
    enum class Mode { soft = 0, hard, digital };

    void prepare (double sampleRate)
    {
        drive.reset (sampleRate, 0.025);
        output.reset (sampleRate, 0.025);
        drive.setCurrentAndTargetValue (1.0f);
        output.setCurrentAndTargetValue (1.0f);
    }

    void setParameters (float driveDb, float outputDb, int newMode) noexcept
    {
        drive.setTargetValue (juce::Decibels::decibelsToGain (driveDb));
        output.setTargetValue (juce::Decibels::decibelsToGain (outputDb));
        mode = static_cast<Mode> (juce::jlimit (0, 2, newMode));
    }

    void process (juce::AudioBuffer<float>& buffer) noexcept
    {
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float d = drive.getNextValue();
            const float o = output.getNextValue();
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                const float x = buffer.getSample (ch, i) * d;
                float y = 0.0f;
                if (mode == Mode::soft) y = std::tanh (x);
                else if (mode == Mode::hard) y = juce::jlimit (-1.0f, 1.0f, x);
                else y = std::round (juce::jlimit (-1.0f, 1.0f, x) * 31.0f) / 31.0f;
                buffer.setSample (ch, i, y * o);
            }
        }
    }

private:
    juce::SmoothedValue<float> drive, output;
    Mode mode { Mode::soft };
};
}

