#include "ReservoirBank.h"

#include <cmath>

namespace RCML
{
void ReservoirBank::prepare (int numChannels, int size, float leak, float inputGain, float feedbackGain)
{
    channelCount = juce::jmax (0, numChannels);
    bankSize = juce::jmax (0, size);
    leakAmount = juce::jlimit (0.0f, 1.0f, leak);
    inputAmount = inputGain;
    feedbackAmount = feedbackGain;

    states.resize (static_cast<size_t> (channelCount * bankSize));
    writePositions.resize (static_cast<size_t> (channelCount));
    reset();
}

void ReservoirBank::reset()
{
    std::fill (states.begin(), states.end(), 0.0f);
    std::fill (writePositions.begin(), writePositions.end(), 0);
}

void ReservoirBank::processSample (int channel, float inputSample) noexcept
{
    if (channel < 0 || channel >= channelCount || bankSize <= 0)
        return;

    inputSample = sanitize (inputSample);

    auto& writePosition = writePositions[static_cast<size_t> (channel)];
    const auto channelOffset = channel * bankSize;
    const auto previousIndex = writePosition > 0 ? writePosition - 1 : bankSize - 1;
    const auto oldValue = states[static_cast<size_t> (channelOffset + writePosition)];
    const auto feedback = states[static_cast<size_t> (channelOffset + previousIndex)];
    const auto driven = inputAmount * inputSample + feedbackAmount * feedback;
    const auto target = softsign (driven);
    const auto nextValue = sanitize (oldValue + leakAmount * (target - oldValue));

    states[static_cast<size_t> (channelOffset + writePosition)] = nextValue;
    writePosition = (writePosition + 1) % bankSize;
}

float ReservoirBank::getState (int channel, int index) const noexcept
{
    if (channel < 0 || channel >= channelCount || index < 0 || index >= bankSize || bankSize <= 0)
        return 0.0f;

    const auto writePosition = writePositions[static_cast<size_t> (channel)];
    const auto newestIndex = writePosition > 0 ? writePosition - 1 : bankSize - 1;
    auto physicalIndex = newestIndex - index;

    while (physicalIndex < 0)
        physicalIndex += bankSize;

    const auto channelOffset = channel * bankSize;
    return states[static_cast<size_t> (channelOffset + physicalIndex)];
}

float ReservoirBank::softsign (float value) noexcept
{
    value = sanitize (value);
    return value / (1.0f + std::abs (value));
}

float ReservoirBank::sanitize (float value) noexcept
{
    return std::isfinite (value) ? juce::jlimit (-8.0f, 8.0f, value) : 0.0f;
}
} // namespace RCML
