#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"

class RCCharacterCaptureFXAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                       private juce::Timer
{
public:
    explicit RCCharacterCaptureFXAudioProcessorEditor (RCCharacterCaptureFXAudioProcessor&);
    ~RCCharacterCaptureFXAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    void timerCallback() override;
    void updateSidechainLabel();
    void updateCaptureStatusLabel();
    void configureGainSlider (juce::Slider& slider, juce::Label& label, const juce::String& text);

    RCCharacterCaptureFXAudioProcessor& audioProcessor;

    juce::ToggleButton bypassButton { "Bypass" };
    juce::ToggleButton learnArmedButton { "Learn Armed" };
    juce::TextButton captureButton { "Capture Last 30s" };
    juce::Slider inputGainSlider;
    juce::Slider outputGainSlider;
    juce::Label inputGainLabel;
    juce::Label outputGainLabel;
    juce::Label sidechainLabel;
    juce::Label captureStatusLabel;

    ButtonAttachment bypassAttachment;
    ButtonAttachment learnArmedAttachment;
    SliderAttachment inputGainAttachment;
    SliderAttachment outputGainAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RCCharacterCaptureFXAudioProcessorEditor)
};
