#pragma once

#include <atomic>

#include <juce_audio_processors/juce_audio_processors.h>

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

    juce::AudioProcessorValueTreeState parameters;

private:
    void updateSidechainState() noexcept;

    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* inputGainDbParam = nullptr;
    std::atomic<float>* outputGainDbParam = nullptr;
    std::atomic<bool> sidechainAvailable { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RCCharacterCaptureFXAudioProcessor)
};
