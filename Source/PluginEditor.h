#pragma once
#include "PluginProcessor.h"
// Temporary editor; replaced in Task 4.
class OctoEqEditor : public juce::GenericAudioProcessorEditor {
public:
    explicit OctoEqEditor(OctoEqProcessor& p) : juce::GenericAudioProcessorEditor(p) {}
};
