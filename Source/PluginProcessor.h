#pragma once

#include <atomic>

#include <juce_audio_processors/juce_audio_processors.h>

#include "Capture/CaptureManager.h"
#include "DSP/AudioEngine.h"
#include "Parameters.h"

class RCCharacterCaptureFXAudioProcessor final : public juce::AudioProcessor
{
public:
    RCCharacterCaptureFXAudioProcessor();
    ~RCCharacterCaptureFXAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    bool isSidechainAvailable() const noexcept;
    int getReservoirFeatureCount() const noexcept;
    bool isReservoirEnabled() const noexcept;
    float getLastReservoirPeak() const noexcept;
    int getReservoirNanDetectedCount() const noexcept;
    RCCapture::CaptureAnalysis captureLast30Seconds();
    juce::String getCaptureStatusText() const;

    juce::AudioProcessorValueTreeState parameters;

private:
    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* inputGainDbParam = nullptr;
    std::atomic<float>* outputGainDbParam = nullptr;
    std::atomic<float>* learnArmedParam = nullptr;
    RCDSP::AudioEngine audioEngine;
    RCCapture::CaptureManager captureManager;
    RCCapture::CaptureAnalysis lastCaptureAnalysis;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RCCharacterCaptureFXAudioProcessor)
};
