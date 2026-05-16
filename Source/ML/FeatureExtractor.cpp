#include "FeatureExtractor.h"

#include <cmath>

namespace RCML
{
void FeatureExtractor::prepare (const ReservoirConfig& config, int)
{
    currentConfig = config;
    currentConfig.sampleRate = currentConfig.sampleRate > 0.0 ? currentConfig.sampleRate : 44100.0;
    channelCount = juce::jmax (1, currentConfig.numChannels);
    inputDelayLineSize = juce::jmax (1, currentConfig.inputDelayLineSamples);

    shortBank.prepare (channelCount, currentConfig.shortBankSize, currentConfig.shortLeak,
                       currentConfig.inputGain, currentConfig.feedbackGain);
    midBank.prepare (channelCount, currentConfig.midBankSize, currentConfig.midLeak,
                     currentConfig.inputGain, currentConfig.feedbackGain);
    slowBank.prepare (channelCount, currentConfig.slowBankSize, currentConfig.slowLeak,
                      currentConfig.inputGain, currentConfig.feedbackGain);

    delayTapSamples.resize (currentConfig.inputDelayTapMs.size());

    for (size_t i = 0; i < currentConfig.inputDelayTapMs.size(); ++i)
    {
        const auto samples = static_cast<int> (std::round (currentConfig.sampleRate
                                                           * static_cast<double> (currentConfig.inputDelayTapMs[i])
                                                           / 1000.0));
        delayTapSamples[i] = juce::jlimit (0, inputDelayLineSize - 1, samples);
    }

    featureCount = 1;

    if (currentConfig.includeInputFeatures)
        featureCount += 1 + static_cast<int> (delayTapSamples.size());

    featureCount += shortBank.getSize() + midBank.getSize() + slowBank.getSize();

    if (currentConfig.includeEnvelopeFeatures)
        featureCount += 2;

    inputDelayLines.resize (static_cast<size_t> (channelCount * inputDelayLineSize));
    inputDelayWritePositions.resize (static_cast<size_t> (channelCount));
    fastEnvelopes.resize (static_cast<size_t> (channelCount));
    slowEnvelopes.resize (static_cast<size_t> (channelCount));
    featureBuffers.resize (static_cast<size_t> (channelCount * featureCount));

    fastEnvelopeCoefficient = calculateEnvelopeCoefficient (currentConfig.sampleRate, currentConfig.fastEnvelopeMs);
    slowEnvelopeCoefficient = calculateEnvelopeCoefficient (currentConfig.sampleRate, currentConfig.slowEnvelopeMs);

    reset();
}

void FeatureExtractor::reset()
{
    shortBank.reset();
    midBank.reset();
    slowBank.reset();
    std::fill (inputDelayLines.begin(), inputDelayLines.end(), 0.0f);
    std::fill (inputDelayWritePositions.begin(), inputDelayWritePositions.end(), 0);
    std::fill (fastEnvelopes.begin(), fastEnvelopes.end(), 0.0f);
    std::fill (slowEnvelopes.begin(), slowEnvelopes.end(), 0.0f);
    std::fill (featureBuffers.begin(), featureBuffers.end(), 0.0f);
}

const float* FeatureExtractor::processSampleAndGetFeatures (int channel, float inputSample) noexcept
{
    if (channel < 0 || channel >= channelCount || featureCount <= 0)
        return nullptr;

    inputSample = sanitize (inputSample);

    auto* features = featureBuffers.data() + static_cast<size_t> (channel * featureCount);
    auto featureIndex = 0;

    features[featureIndex++] = 1.0f;

    if (currentConfig.includeInputFeatures)
    {
        features[featureIndex++] = inputSample;

        for (const auto delaySamples : delayTapSamples)
            features[featureIndex++] = readDelayedInput (channel, delaySamples);
    }

    const auto reservoirInput = inputSample * currentConfig.nonlinearity;

    shortBank.processSample (channel, reservoirInput);
    midBank.processSample (channel, reservoirInput);
    slowBank.processSample (channel, reservoirInput);

    fillBankFeatures (features, featureIndex, shortBank, channel);
    fillBankFeatures (features, featureIndex, midBank, channel);
    fillBankFeatures (features, featureIndex, slowBank, channel);

    const auto magnitude = std::abs (inputSample);
    auto& fastEnvelope = fastEnvelopes[static_cast<size_t> (channel)];
    auto& slowEnvelope = slowEnvelopes[static_cast<size_t> (channel)];

    fastEnvelope = sanitize (fastEnvelope + fastEnvelopeCoefficient * (magnitude - fastEnvelope));
    slowEnvelope = sanitize (slowEnvelope + slowEnvelopeCoefficient * (magnitude - slowEnvelope));

    if (currentConfig.includeEnvelopeFeatures)
    {
        features[featureIndex++] = fastEnvelope;
        features[featureIndex++] = slowEnvelope;
    }

    auto& writePosition = inputDelayWritePositions[static_cast<size_t> (channel)];
    inputDelayLines[static_cast<size_t> (channel * inputDelayLineSize + writePosition)] = inputSample;
    writePosition = (writePosition + 1) % inputDelayLineSize;

    return features;
}

float FeatureExtractor::sanitize (float value) noexcept
{
    return std::isfinite (value) ? juce::jlimit (-8.0f, 8.0f, value) : 0.0f;
}

float FeatureExtractor::calculateEnvelopeCoefficient (double sampleRate, float timeMs) noexcept
{
    const auto safeSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    const auto safeTimeSeconds = juce::jmax (0.001f, timeMs * 0.001f);
    return static_cast<float> (1.0 - std::exp (-1.0 / (safeSampleRate * safeTimeSeconds)));
}

void FeatureExtractor::fillBankFeatures (float* destination,
                                         int& featureIndex,
                                         const ReservoirBank& bank,
                                         int channel) const noexcept
{
    for (auto i = 0; i < bank.getSize(); ++i)
        destination[featureIndex++] = bank.getState (channel, i);
}

float FeatureExtractor::readDelayedInput (int channel, int delaySamples) const noexcept
{
    if (channel < 0 || channel >= channelCount || inputDelayLineSize <= 0)
        return 0.0f;

    const auto writePosition = inputDelayWritePositions[static_cast<size_t> (channel)];
    auto readPosition = writePosition - delaySamples - 1;

    while (readPosition < 0)
        readPosition += inputDelayLineSize;

    return inputDelayLines[static_cast<size_t> (channel * inputDelayLineSize + readPosition)];
}
} // namespace RCML
