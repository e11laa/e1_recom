cmake_minimum_required(VERSION 3.22)

project(RCCharacterCaptureFX VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(RC_LOCAL_JUCE_CONFIG_DIR "C:/Program Files/Common Files/VST3/lib/cmake/JUCE-8.0.6")

if(EXISTS "${RC_LOCAL_JUCE_CONFIG_DIR}/JUCEConfig.cmake"
   AND EXISTS "${RC_LOCAL_JUCE_CONFIG_DIR}/VST3_HELPER.cmake"
   AND EXISTS "${RC_LOCAL_JUCE_CONFIG_DIR}/LV2_HELPER.cmake")
    list(APPEND CMAKE_PREFIX_PATH "C:/Program Files/Common Files/VST3")
    find_package(JUCE CONFIG REQUIRED)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.12
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(RCCharacterCaptureFX
    COMPANY_NAME "affog"
    DESCRIPTION "Stereo passthrough effect with optional sidechain reference input"
    PLUGIN_MANUFACTURER_CODE Affg
    PLUGIN_CODE Rcfx
    PLUGIN_NAME "RCCharacterCaptureFX"
    PRODUCT_NAME "RCCharacterCaptureFX"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_COPY_DIR "C:/Program Files/Common Files/VST3"
    FORMATS VST3)

target_sources(RCCharacterCaptureFX
    PRIVATE
        Source/Parameters.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h)

target_compile_definitions(RCCharacterCaptureFX
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(RCCharacterCaptureFX
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
