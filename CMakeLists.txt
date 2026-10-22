cmake_minimum_required(VERSION 3.22)
project(ProximityEffect VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")

# Use a local JUCE checkout with -DJUCE_PATH=..., otherwise download it.
set(JUCE_PATH "" CACHE PATH "Path to a local JUCE checkout")
if(JUCE_PATH)
    add_subdirectory(${JUCE_PATH} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

set(PE_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND PE_FORMATS AU)
endif()

juce_add_plugin(ProximityEffect
    COMPANY_NAME "Sambito"
    PLUGIN_MANUFACTURER_CODE Smbt
    PLUGIN_CODE Prxe
    FORMATS ${PE_FORMATS}
    PRODUCT_NAME "Proximity Effect"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx EQ Distortion
    AU_MAIN_TYPE kAudioUnitType_Effect)

target_sources(ProximityEffect PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(ProximityEffect PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(ProximityEffect
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

option(PE_BUILD_TOOLS "Build the developer check tool (DSP measurements + UI snapshots)" OFF)
if(PE_BUILD_TOOLS)
    juce_add_console_app(DevCheck PRODUCT_NAME "DevCheck")
    target_sources(DevCheck PRIVATE
        Tools/DevCheck.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)
    target_compile_definitions(DevCheck PRIVATE
        JucePlugin_Name="Proximity Effect"
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0)
    target_link_libraries(DevCheck PRIVATE juce::juce_audio_utils juce::juce_dsp
        PUBLIC juce::juce_recommended_config_flags)
endif()
