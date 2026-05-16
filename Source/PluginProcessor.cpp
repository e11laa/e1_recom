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

    jassert (bypassParam != nullptr);
    jassert (inputGainDbParam != nullptr);
    jassert (outputGainDbParam != nullptr);
}

void RCCharacterCaptureFXAudioProcessor::prepareToPlay (double, int)
{
    updateSidechainState();
}

void RCCharacterCaptureFXAudioProcessor::releaseResources()
{
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

    updateSidechainState();

    auto mainInput = getBusBuffer (buffer, true, 0);
    auto mainOutput = getBusBuffer (buffer, false, 0);

    const auto mainChannelsToCopy = juce::jmin (mainInput.getNumChannels(), mainOutput.getNumChannels());
    const auto numSamples = buffer.getNumSamples();

    for (auto channel = 0; channel < mainChannelsToCopy; ++channel)
        mainOutput.copyFrom (channel, 0, mainInput, channel, 0, numSamples);

    for (auto channel = mainChannelsToCopy; channel < mainOutput.getNumChannels(); ++channel)
        mainOutput.clear (channel, 0, numSamples);

    if (mainChannelsToCopy == 0)
    {
        mainOutput.clear();
        return;
    }

    const auto bypassed = bypassParam != nullptr && bypassParam->load() >= 0.5f;

    if (bypassed)
        return;

    const auto inputGain = juce::Decibels::decibelsToGain (inputGainDbParam != nullptr
                                                               ? inputGainDbParam->load()
                                                               : 0.0f);
    const auto outputGain = juce::Decibels::decibelsToGain (outputGainDbParam != nullptr
                                                                ? outputGainDbParam->load()
                                                                : 0.0f);
    const auto gain = inputGain * outputGain;

    if (gain != 1.0f)
    {
        for (auto channel = 0; channel < mainChannelsToCopy; ++channel)
            mainOutput.applyGain (channel, 0, numSamples, gain);
    }
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
    return sidechainAvailable.load();
}

void RCCharacterCaptureFXAudioProcessor::updateSidechainState() noexcept
{
    const auto* sidechainBus = getBus (true, 1);
    const auto hasChannels = sidechainBus != nullptr
        && sidechainBus->isEnabled()
        && sidechainBus->getNumberOfChannels() > 0;

    sidechainAvailable.store (hasChannels);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RCCharacterCaptureFXAudioProcessor();
}
