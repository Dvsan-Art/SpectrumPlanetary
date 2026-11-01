cmake_minimum_required(VERSION 3.22)
project(SpectrumPlanetary VERSION 1.0.0)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(SpectrumPlanetary
    COMPANY_NAME "MiEstudio"
    PLUGIN_MANUFACTURER_CODE Mest
    PLUGIN_CODE Spp1
    FORMATS VST3 Standalone
    PRODUCT_NAME "Spectrum Planetary"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(SpectrumPlanetary PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(SpectrumPlanetary PUBLIC
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(SpectrumPlanetary
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)
