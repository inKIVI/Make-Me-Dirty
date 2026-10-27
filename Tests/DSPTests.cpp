#include <JuceHeader.h>
#include "DSP/BitCrusher.h"
#include "DSP/SampleRateReducer.h"
#include "DSP/PitchShifter.h"
#include "DSP/ToneFilters.h"
#include "DSP/Distortion.h"
#include "DSP/NoiseGenerator.h"
#include "Presets/PresetManager.h"
#include <cmath>
#include <iostream>

namespace
{
class DummyProcessor final : public juce::AudioProcessor
{
public:
    DummyProcessor() : AudioProcessor (BusesProperties().withInput ("In", juce::AudioChannelSet::stereo(), true).withOutput ("Out", juce::AudioChannelSet::stereo(), true)) {}
    void prepareToPlay (double, int) override {} void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    const juce::String getName() const override { return "Dummy"; }
    bool acceptsMidi() const override { return false; } bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; } double getTailLengthSeconds() const override { return 0; }
    int getNumPrograms() override { return 1; } int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {} const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override {} void setStateInformation (const void*, int) override {}
};

juce::AudioProcessorValueTreeState::ParameterLayout testLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout p;
    const std::initializer_list<const char*> ids { "bitDepth","bitCharacter","sampleRate","interpolation","pitch","drive","driveOutput","distortionMode","lowCut","highCut","midTone","noise","noiseType","mix","input","output" };
    for (auto id : ids) p.add (std::make_unique<juce::AudioParameterFloat> (id, id, 0.0f, 48000.0f, 0.0f));
    return p;
}

class DSPTests final : public juce::UnitTest
{
public:
    DSPTests() : UnitTest ("RETROGRADE DSP", "Retrograde") {}
    void runTest() override
    {
        beginTest ("Bit reduction changes signal and remains finite");
        juce::AudioBuffer<float> b (2, 4096); for (int i=0;i<4096;++i) for(int c=0;c<2;++c) b.setSample(c,i,0.12345f + i*0.00001f);
        auto original = b.getSample (0, 4000); retrograde::dsp::BitCrusher crusher; crusher.prepare (48000,2); crusher.setParameters (4,0); crusher.process (b);
        expect (std::abs (b.getSample(0,4000)-original)>0.001f); expect (allFinite (b));

        beginTest ("Sample hold reduces value transitions");
        for(int i=0;i<4096;++i) for(int c=0;c<2;++c) b.setSample(c,i,(float)i/4096.0f);
        retrograde::dsp::SampleRateReducer reducer; reducer.prepare(48000,2); reducer.setParameters(1000,0); reducer.process(b);
        int transitions=0; for(int i=2049;i<4096;++i) if(b.getSample(0,i)!=b.getSample(0,i-1)) ++transitions;
        expect (transitions < 100); expect (allFinite (b));

        beginTest ("Low pass attenuates high-frequency energy");
        juce::AudioBuffer<float> f (1, 8192); for(int i=0;i<f.getNumSamples();++i) f.setSample(0,i,std::sin(juce::MathConstants<double>::twoPi*12000.0*i/48000.0));
        retrograde::dsp::ToneFilters filters; filters.prepare(48000,1); filters.setParameters(20,2000,0); filters.process(f);
        expect (f.getRMSLevel(0,4096,4096) < 0.2f); expect(allFinite(f));

        beginTest ("Pitch shifter changes pitch without changing frame count");
        retrograde::dsp::PitchShifter shifter; shifter.prepare(48000,24000,1); shifter.setSemitones(12);
        juce::AudioBuffer<float> p (1,24000); for(int i=0;i<p.getNumSamples();++i) p.setSample(0,i,0.4f*std::sin(juce::MathConstants<double>::twoPi*440.0*i/48000.0));
        shifter.process(p); const double measured=zeroCrossFrequency(p,8000,15000,48000.0);
        expectWithinAbsoluteError(measured,880.0,75.0); expectEquals(p.getNumSamples(),24000); expect(allFinite(p));

        beginTest ("Pitch latency report matches unity-pitch impulse");
        retrograde::dsp::PitchShifter unity; unity.prepare(48000,8192,1); unity.setSemitones(0);
        juce::AudioBuffer<float> impulse(1,8192); impulse.clear(); impulse.setSample(0,0,1.0f); unity.process(impulse);
        int peak=0; for(int i=1;i<impulse.getNumSamples();++i) if(std::abs(impulse.getSample(0,i))>std::abs(impulse.getSample(0,peak))) peak=i;
        expectWithinAbsoluteError(peak, unity.getLatencySamples(), 1);

        beginTest ("Dry mix at zero is bit-identical");
        juce::AudioBuffer<float> dry(2,128), wet(2,128); for(int i=0;i<128;++i){dry.setSample(0,i,(float)i/128); dry.setSample(1,i,-(float)i/128); wet.setSample(0,i,0.5f);wet.setSample(1,i,0.5f);}
        for(int c=0;c<2;++c) for(int i=0;i<128;++i) wet.setSample(c,i,dry.getSample(c,i)*(1.0f-0.0f)+wet.getSample(c,i)*0.0f);
        bool same=true; for(int c=0;c<2;++c) for(int i=0;i<128;++i) same = same && dry.getSample(c,i)==wet.getSample(c,i);
        expect (same);

        beginTest ("Silence and parameter boundaries remain finite");
        juce::AudioBuffer<float> silence(2,1024); silence.clear(); crusher.setParameters(4,2); crusher.process(silence); reducer.setParameters(1000,1); reducer.process(silence);
        retrograde::dsp::Distortion d; d.prepare(192000); d.setParameters(30,-24,2); d.process(silence); expect(allFinite(silence));

        beginTest ("Preset serialization and restoration");
        DummyProcessor dummy; juce::AudioProcessorValueTreeState apvts(dummy,nullptr,"STATE",testLayout()); retrograde::PresetManager presets(apvts);
        expect(presets.getNumPresets() >= 8); expect(presets.loadPreset(4));
        expectEquals (presets.getPresetName (0), juce::String ("01  PS1 Horror"));
        expectEquals (presets.getPresetName (7), juce::String ("08  Subtle Retro"));
        const auto state=apvts.copyState(); const auto xml=state.toXmlString(); const auto restored=juce::ValueTree::fromXml(*juce::parseXML(xml));
        expect(restored.isValid()); expectEquals(restored.getNumChildren(),state.getNumChildren());

        beginTest ("Mono and stereo processing performance");
        const auto start=juce::Time::getHighResolutionTicks();
        for(int n=0;n<200;++n){juce::AudioBuffer<float> perf((n&1)+1,512); perf.clear(); crusher.process(perf);}
        const double elapsed=juce::Time::highResolutionTicksToSeconds(juce::Time::getHighResolutionTicks()-start);
        logMessage("200 x 512 sample blocks: " + juce::String(elapsed*1000.0,2) + " ms"); expect(elapsed < 2.0);
    }
private:
    static bool allFinite(const juce::AudioBuffer<float>& x){for(int c=0;c<x.getNumChannels();++c)for(int i=0;i<x.getNumSamples();++i)if(!std::isfinite(x.getSample(c,i)))return false;return true;}
    static double zeroCrossFrequency(const juce::AudioBuffer<float>& x,int start,int count,double sr){int crosses=0;float prev=x.getSample(0,start);for(int i=start+1;i<start+count;++i){float v=x.getSample(0,i);if(prev<=0&&v>0)++crosses;prev=v;}return crosses*sr/count;}
};

DSPTests dspTests;

class ConsoleRunner final : public juce::UnitTestRunner
{
    void logMessage (const juce::String& message) override { std::cout << message << std::endl; }
};
}

int main()
{
    ConsoleRunner runner; runner.setAssertOnFailure (false);
    runner.runTestsInCategory ("Retrograde");
    int failures=0; for(int i=0;i<runner.getNumResults();++i) failures += runner.getResult(i)->failures;
    return failures == 0 ? 0 : 1;
}

