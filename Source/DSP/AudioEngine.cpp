#include "AudioEngine.h"

namespace RCDSP
{
void AudioEngine::prepare (double sampleRate, int maxBlockSize, int numChannels)
{
    preparedChannels = juce::jmax (0, numChannels);
    preparedMaxBlockSize = juce::jmax (0, maxBlockSize);

    inputGain.reset (sampleRate, 0.03);
    outputGain.reset (sampleRate, 0.03);
    inputGain.setCurrentAndTargetValue (1.0f);
    outputGain.setCurrentAndTargetValue (1.0f);

    dcBlocker.prepare (sampleRate, preparedChannels);
    softLimiter.reset();
    sidechainPresent.store (false);
}

void AudioEngine::reset()
{
    inputGain.setCurrentAndTargetValue (inputGain.getTargetValue());
    outputGain.setCurrentAndTargetValue (outputGain.getTargetValue());
    dcBlocker.reset();
    softLimiter.reset();
    sidechainPresent.store (false);
}

void AudioEngine::setBypass (bool enabled) noexcept
{
    bypassed = enabled;
}

void AudioEngine::setInputGainDb (float db) noexcept
{
    inputGain.setTargetValue (dbToGain (db));
}

void AudioEngine::setOutputGainDb (float db) noexcept
{
    outputGain.setTargetValue (dbToGain (db));
}

void AudioEngine::processBlock (juce::AudioBuffer<float>& mainBuffer,
                                const juce::AudioBuffer<float>* sidechainBuffer) noexcept
{
    updateSidechainState (sidechainBuffer);

    const auto numSamples = mainBuffer.getNumSamples();
    const auto numChannels = juce::jmin (mainBuffer.getNumChannels(), preparedChannels);

    if (bypassed || numSamples <= 0 || numChannels <= 0)
    {
        inputGain.skip (numSamples);
        outputGain.skip (numSamples);
        return;
    }

    for (auto sample = 0; sample < numSamples; ++sample)
    {
        const auto currentInputGain = inputGain.getNextValue();
        const auto currentOutputGain = outputGain.getNextValue();

        for (auto channel = 0; channel < numChannels; ++channel)
        {
            auto* samples = mainBuffer.getWritePointer (channel);
            auto value = samples[sample] * currentInputGain;

            value = dcBlocker.processSample (channel, value);
            value = softLimiter.processSample (value);
            samples[sample] = value * currentOutputGain;
        }
    }
}

bool AudioEngine::getSidechainPresent() const noexcept
{
    return sidechainPresent.load();
}

float AudioEngine::dbToGain (float db) noexcept
{
    return juce::Decibels::decibelsToGain (juce::jlimit (-96.0f, 36.0f, db));
}

void AudioEngine::updateSidechainState (const juce::AudioBuffer<float>* sidechainBuffer) noexcept
{
    const auto present = sidechainBuffer != nullptr
        && sidechainBuffer->getNumChannels() > 0
        && sidechainBuffer->getNumSamples() > 0;

    sidechainPresent.store (present);
}
} // namespace RCDSP
