#include "PluginProcessor.h"

#include "PluginEditor.h"

RCCharacterCaptureFXAudioProcessor::RCCharacterCaptureFXAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", RCParameters::createParameterLayout())
{
    bypassParam = parameters.getRawParameterValue (RCParameters::bypassId);
    inputGainDbParam = parameters.getRawParameterValue (RCParameters::inputGainDbId);
    outputGainDbParam = parameters.getRawParameterValue (RCParameters::outputGainDbId);
    learnArmedParam = parameters.getRawParameterValue (RCParameters::learnArmedId);

    jassert (bypassParam != nullptr);
    jassert (inputGainDbParam != nullptr);
    jassert (outputGainDbParam != nullptr);
    jassert (learnArmedParam != nullptr);
}

void RCCharacterCaptureFXAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    audioEngine.prepare (sampleRate, samplesPerBlock, getMainBusNumOutputChannels());
    captureManager.prepare (sampleRate, samplesPerBlock, getMainBusNumOutputChannels(), 30.0);
}

void RCCharacterCaptureFXAudioProcessor::releaseResources()
{
    audioEngine.reset();
    captureManager.reset();
}

bool RCCharacterCaptureFXAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mainInput = layouts.getMainInputChannelSet();
    const auto mainOutput = layouts.getMainOutputChannelSet();

    if (mainInput != juce::AudioChannelSet::stereo()
        || mainOutput != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.inputBuses.size() < 2)
        return true;

    const auto sidechain = layouts.inputBuses[1];
    return sidechain.isDisabled()
        || sidechain == juce::AudioChannelSet::mono()
        || sidechain == juce::AudioChannelSet::stereo();
}

void RCCharacterCaptureFXAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                       juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    midiMessages.clear();

    auto mainOutput = getBusBuffer (buffer, false, 0);
    juce::AudioBuffer<float> sidechainInput;
    const juce::AudioBuffer<float>* sidechainInputPtr = nullptr;

    if (const auto* sidechainBus = getBus (true, 1);
        sidechainBus != nullptr && sidechainBus->isEnabled())
    {
        sidechainInput = getBusBuffer (buffer, true, 1);
        sidechainInputPtr = &sidechainInput;
    }

    captureManager.setLearnArmed (learnArmedParam != nullptr && learnArmedParam->load() >= 0.5f);
    captureManager.pushAudioBlock (mainOutput, sidechainInputPtr, mainOutput.getNumSamples());

    audioEngine.setBypass (bypassParam != nullptr && bypassParam->load() >= 0.5f);
    audioEngine.setInputGainDb (inputGainDbParam != nullptr ? inputGainDbParam->load() : 0.0f);
    audioEngine.setOutputGainDb (outputGainDbParam != nullptr ? outputGainDbParam->load() : 0.0f);
    audioEngine.processBlock (mainOutput, sidechainInputPtr);

    for (auto channel = getMainBusNumInputChannels(); channel < mainOutput.getNumChannels(); ++channel)
        mainOutput.clear (channel, 0, mainOutput.getNumSamples());
}

juce::AudioProcessorEditor* RCCharacterCaptureFXAudioProcessor::createEditor()
{
    return new RCCharacterCaptureFXAudioProcessorEditor (*this);
}

bool RCCharacterCaptureFXAudioProcessor::hasEditor() const
{
    return true;
}

const juce::String RCCharacterCaptureFXAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool RCCharacterCaptureFXAudioProcessor::acceptsMidi() const
{
    return false;
}

bool RCCharacterCaptureFXAudioProcessor::producesMidi() const
{
    return false;
}

bool RCCharacterCaptureFXAudioProcessor::isMidiEffect() const
{
    return false;
}

double RCCharacterCaptureFXAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int RCCharacterCaptureFXAudioProcessor::getNumPrograms()
{
    return 1;
}

int RCCharacterCaptureFXAudioProcessor::getCurrentProgram()
{
    return 0;
}

void RCCharacterCaptureFXAudioProcessor::setCurrentProgram (int)
{
}

const juce::String RCCharacterCaptureFXAudioProcessor::getProgramName (int)
{
    return {};
}

void RCCharacterCaptureFXAudioProcessor::changeProgramName (int, const juce::String&)
{
}

void RCCharacterCaptureFXAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = parameters.copyState().createXml())
        copyXmlToBinary (*state, destData);
}

void RCCharacterCaptureFXAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto state = getXmlFromBinary (data, sizeInBytes))
    {
        if (state->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*state));
    }
}

bool RCCharacterCaptureFXAudioProcessor::isSidechainAvailable() const noexcept
{
    return audioEngine.getSidechainPresent();
}

int RCCharacterCaptureFXAudioProcessor::getReservoirFeatureCount() const noexcept
{
    return audioEngine.getReservoirFeatureCount();
}

bool RCCharacterCaptureFXAudioProcessor::isReservoirEnabled() const noexcept
{
    return audioEngine.isReservoirEnabled();
}

float RCCharacterCaptureFXAudioProcessor::getLastReservoirPeak() const noexcept
{
    return audioEngine.getLastReservoirPeak();
}

int RCCharacterCaptureFXAudioProcessor::getReservoirNanDetectedCount() const noexcept
{
    return audioEngine.getReservoirNanDetectedCount();
}

RCCapture::CaptureAnalysis RCCharacterCaptureFXAudioProcessor::captureLast30Seconds()
{
    const auto snapshot = captureManager.createSnapshotLast (30.0);
    lastCaptureAnalysis = captureManager.analyzeSnapshot (snapshot);
    return lastCaptureAnalysis;
}

juce::String RCCharacterCaptureFXAudioProcessor::getCaptureStatusText() const
{
    if (captureManager.isLearnArmed())
        return "Capture: Recording";

    if (lastCaptureAnalysis.statusText != "Capture: Empty")
        return lastCaptureAnalysis.statusText;

    return captureManager.hasCapturedAudio() ? "Capture: Ready" : "Capture: Empty";
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RCCharacterCaptureFXAudioProcessor();
}
