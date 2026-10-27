#pragma once

#include <JuceHeader.h>
#include <vector>

namespace retrograde
{
class PresetManager
{
public:
    explicit PresetManager (juce::AudioProcessorValueTreeState&);

    int getNumPresets() const noexcept { return (int) presets.size(); }
    int getCurrentPresetIndex() const noexcept { return currentIndex; }
    juce::String getCurrentPresetName() const;
    juce::String getPresetName (int index) const;
    bool isFactoryPreset (int index) const;

    bool loadPreset (int index);
    void loadPrevious();
    void loadNext();
    bool saveCustomPreset (const juce::String& name);
    bool renameCurrentPreset (const juce::String& name);
    bool deleteCurrentPreset();
    void restoreFactoryPresets();
    void setCurrentPresetName (const juce::String&);

    std::function<void()> onPresetListChanged;

private:
    struct Preset
    {
        juce::String name;
        juce::ValueTree state;
        bool factory { false };
    };

    void createFactoryPresets();
    void addFactory (const juce::String&, std::initializer_list<std::pair<const char*, float>>);
    void loadCustomPresets();
    void writeCustomPresets() const;
    juce::File getPresetFile() const;
    void applyState (const juce::ValueTree&);

    juce::AudioProcessorValueTreeState& apvts;
    std::vector<Preset> presets;
    int currentIndex { 0 };
    juce::String transientName { "01  PS1 Horror" };
};
}

