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
