cmake_minimum_required(VERSION 3.22)
project(MCOM VERSION 1.0.0)

# JUCE 7 can be supplied through -DJUCE_DIR=... or as a sibling folder named JUCE.
if (NOT DEFINED JUCE_DIR)
    set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE" CACHE PATH "Path to JUCE")
endif()

add_subdirectory("${JUCE_DIR}" JUCE)

juce_add_plugin(MCOM
    COMPANY_NAME "MCOM Audio"
    BUNDLE_ID "com.mcom.audio.mcom"
    PLUGIN_MANUFACTURER_CODE MCom
    PLUGIN_CODE MCOM
    FORMATS VST3
    PRODUCT_NAME "MCOM"
    DESCRIPTION "Program-dependent optical compressor"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(MCOM)

target_sources(MCOM PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_features(MCOM PRIVATE cxx_std_17)
target_compile_definitions(MCOM PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(MCOM PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
