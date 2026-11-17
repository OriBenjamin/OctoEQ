cmake_minimum_required(VERSION 3.22)
project(OctoEQ VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(OCTOEQ_TESTS_ONLY "Build only the JUCE-free DSP tests" OFF)

add_library(octoeq_dsp STATIC Source/dsp/Biquad.cpp)
target_include_directories(octoeq_dsp PUBLIC Source)
set_target_properties(octoeq_dsp PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_executable(octoeq_tests tests/dsp_tests.cpp)
target_link_libraries(octoeq_tests PRIVATE octoeq_dsp)
target_sources(octoeq_dsp PRIVATE Source/dsp/EqEngine.cpp)
add_executable(octoeq_engine_tests tests/engine_tests.cpp)
target_link_libraries(octoeq_engine_tests PRIVATE octoeq_dsp)

if(NOT OCTOEQ_TESTS_ONLY)
    set(OCTOEQ_JUCE_DIR "" CACHE PATH "Local JUCE checkout (optional; fetched otherwise)")
    if(OCTOEQ_JUCE_DIR)
        add_subdirectory(${OCTOEQ_JUCE_DIR} JUCE)
    else()
        include(FetchContent)
        FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.4 GIT_SHALLOW TRUE)
        FetchContent_MakeAvailable(JUCE)
    endif()

    juce_add_plugin(OctoEQ
        COMPANY_NAME "Octo"
        BUNDLE_ID "com.octo.octoeq"
        PLUGIN_MANUFACTURER_CODE Octo
        PLUGIN_CODE Ocqe
        FORMATS AU VST3
        PRODUCT_NAME "OctoEQ"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
        AU_MAIN_TYPE kAudioUnitType_Effect
        VST3_CATEGORIES Fx EQ)

    target_sources(OctoEQ PRIVATE Source/PluginProcessor.cpp)
    target_compile_definitions(OctoEQ PUBLIC
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)
    target_link_libraries(OctoEQ
        PRIVATE octoeq_dsp juce::juce_audio_utils juce::juce_dsp
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

    add_executable(octoeq_proc_tests tests/processor_tests.cpp)
    target_link_libraries(octoeq_proc_tests PRIVATE OctoEQ octoeq_dsp juce::juce_audio_utils juce::juce_dsp)
    target_include_directories(octoeq_proc_tests PRIVATE Source)
endif()
