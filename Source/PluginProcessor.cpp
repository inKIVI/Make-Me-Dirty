#include "PluginProcessor.h"
#include "PluginEditor.h"

// The reported latency is derived from the pitch shifter's unity-ratio grain centre.

namespace IDs
{
constexpr auto input = "input"; constexpr auto output = "output"; constexpr auto mix = "mix";
constexpr auto bypass = "bypass"; constexpr auto bitDepth = "bitDepth"; constexpr auto bitCharacter = "bitCharacter";
constexpr auto sampleRate = "sampleRate"; constexpr auto interpolation = "interpolation";
constexpr auto pitch = "pitch"; constexpr auto drive = "drive"; constexpr auto driveOutput = "driveOutput";
constexpr auto distortionMode = "distortionMode"; constexpr auto lowCut = "lowCut"; constexpr auto highCut = "highCut";
constexpr auto midTone = "midTone"; constexpr auto noise = "noise"; constexpr auto noiseType = "noiseType";
}

RetrogradeFXAudioProcessor::RetrogradeFXAudioProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "RETROGRADE_STATE", createParameterLayout()),
      presetManager (parameters)
{
}

juce::AudioProcessorValueTreeState::ParameterLayout RetrogradeFXAudioProcessor::createParameterLayout()
{
    using F = juce::AudioParameterFloat; using C = juce::AudioParameterChoice; using B = juce::AudioParameterBool;
    juce::AudioProcessorValueTreeState::ParameterLayout p;
    p.add (std::make_unique<F> (IDs::input, "Input", juce::NormalisableRange<float> (-24, 24, 0.1f), 0.0f, "dB"));
    p.add (std::make_unique<F> (IDs::output, "Output", juce::NormalisableRange<float> (-24, 12, 0.1f), 0.0f, "dB"));
    p.add (std::make_unique<F> (IDs::mix, "Dry / Wet", juce::NormalisableRange<float> (0, 100, 0.1f), 100.0f, "%"));
    p.add (std::make_unique<B> (IDs::bypass, "Bypass", false));
    p.add (std::make_unique<F> (IDs::bitDepth, "Bit Depth", juce::NormalisableRange<float> (4, 16, 0.1f), 12.0f, "bit"));
    p.add (std::make_unique<C> (IDs::bitCharacter, "Quantizer", juce::StringArray { "Clean", "Dither", "Asymmetric" }, 0));
    p.add (std::make_unique<F> (IDs::sampleRate, "Sample Rate", juce::NormalisableRange<float> (1000, 48000, 1, 0.35f), 22050.0f, "Hz"));
    p.add (std::make_unique<C> (IDs::interpolation, "Interpolation", juce::StringArray { "Hold", "Linear" }, 0));
    p.add (std::make_unique<F> (IDs::pitch, "Pitch", juce::NormalisableRange<float> (-12, 12, 0.01f), 0.0f, "st"));
    p.add (std::make_unique<F> (IDs::drive, "Drive", juce::NormalisableRange<float> (0, 30, 0.1f), 4.0f, "dB"));
    p.add (std::make_unique<F> (IDs::driveOutput, "Drive Trim", juce::NormalisableRange<float> (-24, 0, 0.1f), -2.0f, "dB"));
    p.add (std::make_unique<C> (IDs::distortionMode, "Distortion", juce::StringArray { "Soft", "Hard", "Digital" }, 0));
    p.add (std::make_unique<F> (IDs::lowCut, "Low Cut", juce::NormalisableRange<float> (20, 2000, 1, 0.35f), 60.0f, "Hz"));
    p.add (std::make_unique<F> (IDs::highCut, "High Cut", juce::NormalisableRange<float> (1500, 20000, 1, 0.35f), 12000.0f, "Hz"));
    p.add (std::make_unique<F> (IDs::midTone, "Mid Tone", juce::NormalisableRange<float> (-12, 12, 0.1f), 1.0f, "dB"));
    p.add (std::make_unique<F> (IDs::noise, "Noise", juce::NormalisableRange<float> (0, 100, 0.1f), 3.0f, "%"));
    p.add (std::make_unique<C> (IDs::noiseType, "Noise Texture", juce::StringArray { "Hiss", "Digital", "Static" }, 0));
    return p;
}

bool RetrogradeFXAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    return (input == juce::AudioChannelSet::mono() || input == juce::AudioChannelSet::stereo())
        && layouts.getMainOutputChannelSet() == input;
}

void RetrogradeFXAudioProcessor::prepareToPlay (double sr, int maximumBlockSize)
{
    const int channels = juce::jmax (1, getTotalNumOutputChannels());
    originalBuffer.setSize (channels, maximumBlockSize, false, true, true);
    dryBuffer.setSize (channels, maximumBlockSize, false, true, true);
    bitCrusher.prepare (sr, channels); rateReducer.prepare (sr, channels);
    pitchShifter.prepare (sr, maximumBlockSize, channels); distortion.prepare (sr);
    filters.prepare (sr, channels); noise.prepare (sr);
    dryDelayLength = juce::jmax (1, pitchShifter.getLatencySamples());
    dryDelay.assign ((size_t) channels, std::vector<float> ((size_t) dryDelayLength, 0.0f));
    dryDelayPosition = 0;
    setLatencySamples (dryDelayLength);
    inputGain.reset (sr, 0.03); outputGain.reset (sr, 0.03); mix.reset (sr, 0.03);
    inputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (value (IDs::input)));
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (value (IDs::output)));
    mix.setCurrentAndTargetValue (value (IDs::mix) * 0.01f);
}

void RetrogradeFXAudioProcessor::releaseResources() {}

void RetrogradeFXAudioProcessor::processDryDelay (const juce::AudioBuffer<float>& source, juce::AudioBuffer<float>& destination, int numSamples) noexcept
{
    for (int i = 0; i < numSamples; ++i)
    {
        for (int ch = 0; ch < source.getNumChannels(); ++ch)
        {
            auto& line = dryDelay[(size_t) ch];
            destination.setSample (ch, i, line[(size_t) dryDelayPosition]);
            line[(size_t) dryDelayPosition] = source.getSample (ch, i);
        }
        if (++dryDelayPosition >= dryDelayLength) dryDelayPosition = 0;
    }
}

void RetrogradeFXAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch) buffer.clear (ch, 0, buffer.getNumSamples());
    const int samples = juce::jmin (buffer.getNumSamples(), originalBuffer.getNumSamples());
    if (samples <= 0) return;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        originalBuffer.copyFrom (ch, 0, buffer, ch, 0, samples);
    inputMeter.store (juce::Decibels::gainToDecibels (buffer.getMagnitude (0, samples), -100.0f));

    processDryDelay (originalBuffer, dryBuffer, samples);
    inputGain.setTargetValue (juce::Decibels::decibelsToGain (value (IDs::input)));
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (value (IDs::output)));
    const bool bypassed = value (IDs::bypass) >= 0.5f;
    mix.setTargetValue (bypassed ? 0.0f : value (IDs::mix) * 0.01f);

    for (int i = 0; i < samples; ++i)
    {
        const float g = inputGain.getNextValue();
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) buffer.setSample (ch, i, buffer.getSample (ch, i) * g);
    }

    pitchShifter.setSemitones (value (IDs::pitch));
    bitCrusher.setParameters (value (IDs::bitDepth), (int) value (IDs::bitCharacter));
    rateReducer.setParameters (value (IDs::sampleRate), (int) value (IDs::interpolation));
    distortion.setParameters (value (IDs::drive), value (IDs::driveOutput), (int) value (IDs::distortionMode));
    filters.setParameters (value (IDs::lowCut), value (IDs::highCut), value (IDs::midTone));
    noise.setParameters (value (IDs::noise), (int) value (IDs::noiseType));
    pitchShifter.process (buffer); bitCrusher.process (buffer); rateReducer.process (buffer);
    distortion.process (buffer); filters.process (buffer); noise.process (buffer);

    for (int i = 0; i < samples; ++i)
    {
        const float wet = mix.getNextValue();
        const float out = bypassed ? 1.0f : outputGain.getNextValue();
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const float y = dryBuffer.getSample (ch, i) * (1.0f - wet) + buffer.getSample (ch, i) * wet;
            buffer.setSample (ch, i, juce::jlimit (-1.0f, 1.0f, y * out));
        }
    }
    outputMeter.store (juce::Decibels::gainToDecibels (buffer.getMagnitude (0, samples), -100.0f));
}

void RetrogradeFXAudioProcessor::getStateInformation (juce::MemoryBlock& data)
{
    auto state = parameters.copyState();
    state.setProperty ("currentPreset", presetManager.getCurrentPresetName(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, data);
}

void RetrogradeFXAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (parameters.state.getType()))
        {
            auto restored = juce::ValueTree::fromXml (*xml);
            const auto name = restored.getProperty ("currentPreset").toString();
            parameters.replaceState (restored);
            presetManager.setCurrentPresetName (name.isNotEmpty() ? name : "HOST STATE");
        }
}

juce::AudioProcessorEditor* RetrogradeFXAudioProcessor::createEditor() { return new RetrogradeFXAudioProcessorEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new RetrogradeFXAudioProcessor(); }

