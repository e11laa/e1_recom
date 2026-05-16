#include "PluginEditor.h"

RCCharacterCaptureFXAudioProcessorEditor::RCCharacterCaptureFXAudioProcessorEditor (
    RCCharacterCaptureFXAudioProcessor& processor)
    : AudioProcessorEditor (&processor),
      audioProcessor (processor),
      bypassAttachment (audioProcessor.parameters, RCParameters::bypassId, bypassButton),
      inputGainAttachment (audioProcessor.parameters, RCParameters::inputGainDbId, inputGainSlider),
      outputGainAttachment (audioProcessor.parameters, RCParameters::outputGainDbId, outputGainSlider)
{
    addAndMakeVisible (bypassButton);

    configureGainSlider (inputGainSlider, inputGainLabel, "Input Gain");
    configureGainSlider (outputGainSlider, outputGainLabel, "Output Gain");

    sidechainLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (sidechainLabel);
    updateSidechainLabel();

    setSize (360, 180);
    startTimerHz (10);
}

void RCCharacterCaptureFXAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
    g.setColour (juce::Colours::white);
    g.setFont (18.0f);
    g.drawText ("RCCharacterCaptureFX", getLocalBounds().removeFromTop (34),
                juce::Justification::centredLeft);
}

void RCCharacterCaptureFXAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (16);
    bounds.removeFromTop (30);

    bypassButton.setBounds (bounds.removeFromTop (28));
    bounds.removeFromTop (8);

    auto inputRow = bounds.removeFromTop (36);
    inputGainLabel.setBounds (inputRow.removeFromLeft (92));
    inputGainSlider.setBounds (inputRow);

    bounds.removeFromTop (8);

    auto outputRow = bounds.removeFromTop (36);
    outputGainLabel.setBounds (outputRow.removeFromLeft (92));
    outputGainSlider.setBounds (outputRow);

    bounds.removeFromTop (12);
    sidechainLabel.setBounds (bounds.removeFromTop (24));
}

void RCCharacterCaptureFXAudioProcessorEditor::timerCallback()
{
    updateSidechainLabel();
}

void RCCharacterCaptureFXAudioProcessorEditor::updateSidechainLabel()
{
    sidechainLabel.setText (audioProcessor.isSidechainAvailable()
                                ? "Sidechain: OK"
                                : "Sidechain: Missing",
                            juce::dontSendNotification);
}

void RCCharacterCaptureFXAudioProcessorEditor::configureGainSlider (juce::Slider& slider,
                                                                    juce::Label& label,
                                                                    const juce::String& text)
{
    slider.setSliderStyle (juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 72, 22);
    slider.setTextValueSuffix (" dB");
    addAndMakeVisible (slider);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (label);
}
