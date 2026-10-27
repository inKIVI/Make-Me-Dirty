cmake_minimum_required(VERSION 3.22)

project(RetrogradeFX VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
    JUCE
    URL https://github.com/juce-framework/JUCE/archive/refs/tags/8.0.4.zip
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE)
FetchContent_MakeAvailable(JUCE)

set(RETROGRADE_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/DSP/BitCrusher.h
    Source/DSP/SampleRateReducer.h
    Source/DSP/PitchShifter.h
    Source/DSP/Distortion.h
    Source/DSP/ToneFilters.h
    Source/DSP/NoiseGenerator.h
    Source/Presets/PresetManager.cpp
    Source/Presets/PresetManager.h
    Source/UI/CustomLookAndFeel.h
    Source/UI/CustomKnob.h
    Source/UI/LevelMeter.h)

set(RETROGRADE_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND RETROGRADE_FORMATS AU)
endif()

juce_add_plugin(RetrogradeFX
    COMPANY_NAME "inkivi"
    BUNDLE_ID "audio.forgottensignal.retrogradefx"
    PLUGIN_MANUFACTURER_CODE FgSg
    PLUGIN_CODE RgFx
    PRODUCT_NAME "MAKE ME DIRTY"
    DESCRIPTION "Retro game audio degradation processor"
    FORMATS ${RETROGRADE_FORMATS}
    VST3_CATEGORIES Fx Distortion
    AU_MAIN_TYPE kAudioUnitType_Effect
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)
juce_generate_juce_header(RetrogradeFX)

target_sources(RetrogradeFX PRIVATE ${RETROGRADE_SOURCES})
target_include_directories(RetrogradeFX PRIVATE Source)
target_compile_definitions(RetrogradeFX PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(RetrogradeFX PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

option(RETROGRADE_BUILD_TESTS "Build DSP and state tests" ON)
if(RETROGRADE_BUILD_TESTS)
    enable_testing()
    juce_add_console_app(RetrogradeTests PRODUCT_NAME "Retrograde Tests")
    juce_generate_juce_header(RetrogradeTests)
    target_sources(RetrogradeTests PRIVATE Tests/DSPTests.cpp Source/Presets/PresetManager.cpp)
    target_include_directories(RetrogradeTests PRIVATE Source)
    target_compile_definitions(RetrogradeTests PRIVATE
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_UNIT_TESTS=1)
    target_link_libraries(RetrogradeTests PRIVATE
        juce::juce_audio_utils juce::juce_dsp
        juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
    add_test(NAME RetrogradeDSP COMMAND RetrogradeTests)
endif()

