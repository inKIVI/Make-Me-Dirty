#include "PresetManager.h"
#include <algorithm>

namespace retrograde
{
PresetManager::PresetManager (juce::AudioProcessorValueTreeState& state) : apvts (state)
{
    createFactoryPresets();
    loadCustomPresets();
    if (! presets.empty()) loadPreset (0);
}

void PresetManager::addFactory (const juce::String& name, std::initializer_list<std::pair<const char*, float>> values)
{
    auto state = apvts.copyState();
    for (const auto& [id, value] : values)
        if (auto child = state.getChildWithProperty ("id", id); child.isValid()) child.setProperty ("value", value, nullptr);
    presets.push_back ({ name, state, true });
}

void PresetManager::createFactoryPresets()
{
    presets.clear();
    addFactory ("01  PS1 Horror", {{"bitDepth",8},{"sampleRate",11025},{"pitch",-1},{"drive",10},{"driveOutput",-7},{"distortionMode",2},{"lowCut",90},{"highCut",6500},{"midTone",-2},{"noise",11},{"noiseType",2},{"mix",100},{"input",0},{"output",0}});
    addFactory ("02  Half-Life 1998", {{"bitDepth",10},{"sampleRate",16000},{"pitch",0},{"drive",8},{"driveOutput",-5},{"distortionMode",1},{"lowCut",70},{"highCut",9000},{"midTone",3},{"noise",4},{"noiseType",1},{"mix",92}});
    addFactory ("03  CS 1.6", {{"bitDepth",12},{"sampleRate",22050},{"pitch",0},{"drive",6},{"driveOutput",-4},{"distortionMode",0},{"lowCut",120},{"highCut",11500},{"midTone",5},{"noise",1.5f},{"mix",82}});
    addFactory ("04  VHS Tape", {{"bitDepth",13},{"bitCharacter",1},{"sampleRate",26000},{"interpolation",1},{"pitch",-0.25f},{"drive",4},{"driveOutput",-3},{"lowCut",80},{"highCut",5200},{"midTone",-3},{"noise",18},{"noiseType",0},{"mix",95}});
    addFactory ("05  Broken Digital", {{"bitDepth",4},{"bitCharacter",2},{"sampleRate",4000},{"pitch",-5},{"drive",22},{"driveOutput",-15},{"distortionMode",2},{"lowCut",180},{"highCut",3800},{"midTone",8},{"noise",12},{"noiseType",1},{"mix",100}});
    addFactory ("06  Old Computer", {{"bitDepth",9},{"sampleRate",12000},{"pitch",0},{"drive",11},{"driveOutput",-7},{"distortionMode",1},{"lowCut",280},{"highCut",5200},{"midTone",7},{"noise",5},{"noiseType",1},{"mix",100}});
    addFactory ("07  Dark Ambient", {{"bitDepth",7},{"sampleRate",8000},{"pitch",-7},{"drive",13},{"driveOutput",-9},{"distortionMode",0},{"lowCut",50},{"highCut",4200},{"midTone",-5},{"noise",20},{"noiseType",2},{"mix",88}});
    addFactory ("08  Subtle Retro", {{"bitDepth",14},{"bitCharacter",1},{"sampleRate",32000},{"interpolation",1},{"pitch",0},{"drive",3},{"driveOutput",-2},{"distortionMode",0},{"lowCut",35},{"highCut",15000},{"midTone",1},{"noise",1.5f},{"noiseType",0},{"mix",55}});
}

void PresetManager::applyState (const juce::ValueTree& state)
{
    for (int i = 0; i < state.getNumChildren(); ++i)
    {
        const auto child = state.getChild (i);
        const auto id = child.getProperty ("id").toString();
        if (auto* parameter = apvts.getParameter (id))
        {
            const float raw = (float) child.getProperty ("value");
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost (parameter->convertTo0to1 (raw));
            parameter->endChangeGesture();
        }
    }
}

bool PresetManager::loadPreset (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) presets.size())) return false;
    applyState (presets[(size_t) index].state);
    currentIndex = index;
    transientName = presets[(size_t) index].name;
    if (onPresetListChanged) onPresetListChanged();
    return true;
}

void PresetManager::loadPrevious() { if (! presets.empty()) loadPreset ((currentIndex - 1 + getNumPresets()) % getNumPresets()); }
void PresetManager::loadNext() { if (! presets.empty()) loadPreset ((currentIndex + 1) % getNumPresets()); }
juce::String PresetManager::getCurrentPresetName() const { return transientName; }
juce::String PresetManager::getPresetName (int index) const { return juce::isPositiveAndBelow (index, getNumPresets()) ? presets[(size_t) index].name : juce::String(); }
bool PresetManager::isFactoryPreset (int index) const { return juce::isPositiveAndBelow (index, getNumPresets()) && presets[(size_t) index].factory; }

bool PresetManager::saveCustomPreset (const juce::String& requestedName)
{
    const auto name = requestedName.trim().substring (0, 64);
    if (name.isEmpty()) return false;
    presets.push_back ({ name, apvts.copyState(), false });
    currentIndex = getNumPresets() - 1;
    transientName = name;
    writeCustomPresets();
    if (onPresetListChanged) onPresetListChanged();
    return true;
}

bool PresetManager::renameCurrentPreset (const juce::String& requestedName)
{
    if (! juce::isPositiveAndBelow (currentIndex, getNumPresets()) || presets[(size_t) currentIndex].factory) return false;
    const auto name = requestedName.trim().substring (0, 64);
    if (name.isEmpty()) return false;
    presets[(size_t) currentIndex].name = name; transientName = name;
    writeCustomPresets();
    if (onPresetListChanged) onPresetListChanged();
    return true;
}

bool PresetManager::deleteCurrentPreset()
{
    if (! juce::isPositiveAndBelow (currentIndex, getNumPresets()) || presets[(size_t) currentIndex].factory) return false;
    presets.erase (presets.begin() + currentIndex);
    currentIndex = juce::jlimit (0, getNumPresets() - 1, currentIndex - 1);
    writeCustomPresets();
    return loadPreset (currentIndex);
}

void PresetManager::restoreFactoryPresets()
{
    presets.erase (std::remove_if (presets.begin(), presets.end(), [] (const Preset& p) { return ! p.factory; }), presets.end());
    getPresetFile().deleteFile();
    loadPreset (0);
}

void PresetManager::setCurrentPresetName (const juce::String& name)
{
    transientName = name;
    currentIndex = -1;
    if (onPresetListChanged) onPresetListChanged();
}

juce::File PresetManager::getPresetFile() const
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("Forgotten Signal").getChildFile ("RETROGRADE FX").getChildFile ("presets.json");
}

void PresetManager::writeCustomPresets() const
{
    juce::Array<juce::var> items;
    for (const auto& p : presets)
        if (! p.factory)
        {
            auto* object = new juce::DynamicObject();
            object->setProperty ("name", p.name);
            object->setProperty ("state", p.state.toXmlString());
            items.add (juce::var (object));
        }
    const auto file = getPresetFile();
    file.getParentDirectory().createDirectory();
    file.replaceWithText (juce::JSON::toString (juce::var (items), true));
}

void PresetManager::loadCustomPresets()
{
    const auto file = getPresetFile();
    if (! file.existsAsFile()) return;
    const auto parsed = juce::JSON::parse (file.loadFileAsString());
    if (auto* array = parsed.getArray())
        for (const auto& item : *array)
        {
            const auto name = item.getProperty ("name", {}).toString();
            const auto xmlText = item.getProperty ("state", {}).toString();
            if (auto xml = juce::parseXML (xmlText))
            {
                const auto state = juce::ValueTree::fromXml (*xml);
                if (name.isNotEmpty() && state.isValid()) presets.push_back ({ name, state, false });
            }
        }
}
}

