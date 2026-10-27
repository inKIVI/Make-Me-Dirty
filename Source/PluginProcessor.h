#pragma once

#include <JuceHeader.h>
#include "DSP/BitCrusher.h"
#include "DSP/SampleRateReducer.h"
#include "DSP/PitchShifter.h"
#include "DSP/Distortion.h"
#include "DSP/ToneFilters.h"
#include "DSP/NoiseGenerator.h"
#include "Presets/PresetManager.h"
#include <atomic>
#include <vector>

class RetrogradeFXAudioProcessor final : public juce::AudioProcessor
{
public:
    RetrogradeFXAudioProcessor();
    ~RetrogradeFXAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return presetManager.getNumPresets(); }
    int getCurrentProgram() override { return presetManager.getCurrentPresetIndex(); }
    void setCurrentProgram (int index) override { presetManager.loadPreset (index); }
    const juce::String getProgramName (int index) override { return presetManager.getPresetName (index); }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState parameters;
    retrograde::PresetManager presetManager;
    std::atomic<float> inputMeter { -100.0f }, outputMeter { -100.0f };

private:
    float value (const char* id) const noexcept { return parameters.getRawParameterValue (id)->load(); }
    void processDryDelay (const juce::AudioBuffer<float>& source, juce::AudioBuffer<float>& destination, int numSamples) noexcept;

    retrograde::dsp::BitCrusher bitCrusher;
    retrograde::dsp::SampleRateReducer rateReducer;
    retrograde::dsp::PitchShifter pitchShifter;
    retrograde::dsp::Distortion distortion;
    retrograde::dsp::ToneFilters filters;
    retrograde::dsp::NoiseGenerator noise;

    juce::AudioBuffer<float> originalBuffer, dryBuffer;
    std::vector<std::vector<float>> dryDelay;
    int dryDelayPosition { 0 }, dryDelayLength { 0 };
    juce::SmoothedValue<float> inputGain, outputGain, mix;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RetrogradeFXAudioProcessor)
};

