#include "CaptureRingBuffer.h"

#include <cmath>

namespace RCCapture
{
void CaptureRingBuffer::prepare (double sampleRate, double maxCaptureSeconds, int numChannels)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    channelCount = juce::jmax (1, numChannels);
    capacitySamples = juce::jmax (1, static_cast<int> (std::ceil (currentSampleRate * maxCaptureSeconds)));

    mainBuffer.setSize (channelCount, capacitySamples, false, true, false);
    sidechainBuffer.setSize (channelCount, capacitySamples, false, true, false);
    sidechainMask.resize (static_cast<size_t> (capacitySamples), 0);

    reset();
}

void CaptureRingBuffer::reset()
{
    mainBuffer.clear();
    sidechainBuffer.clear();
    std::fill (sidechainMask.begin(), sidechainMask.end(), static_cast<uint8_t> (0));
    writePosition.store (0);
    totalSamplesWritten.store (0);
}

void CaptureRingBuffer::pushBlock (const juce::AudioBuffer<float>& main,
                                   const juce::AudioBuffer<float>* sidechain,
                                   int numSamples) noexcept
{
    if (capacitySamples <= 0 || channelCount <= 0 || numSamples <= 0)
        return;

    const auto samplesToWrite = juce::jmin (numSamples, main.getNumSamples());
    auto localWritePosition = writePosition.load();
    const auto hasSidechain = sidechain != nullptr
        && sidechain->getNumChannels() > 0
        && sidechain->getNumSamples() >= samplesToWrite;

    auto remaining = samplesToWrite;
    auto sourceOffset = 0;

    while (remaining > 0)
    {
        const auto contiguous = juce::jmin (remaining, capacitySamples - localWritePosition);

        for (auto channel = 0; channel < channelCount; ++channel)
        {
            if (channel < main.getNumChannels())
            {
                mainBuffer.copyFrom (channel, localWritePosition, main, channel, sourceOffset, contiguous);
            }
            else
            {
                mainBuffer.clear (channel, localWritePosition, contiguous);
            }

            if (hasSidechain && channel < sidechain->getNumChannels())
            {
                sidechainBuffer.copyFrom (channel, localWritePosition, *sidechain, channel, sourceOffset, contiguous);
            }
            else
            {
                sidechainBuffer.clear (channel, localWritePosition, contiguous);
            }
        }

        std::fill_n (sidechainMask.data() + localWritePosition,
                     static_cast<size_t> (contiguous),
                     static_cast<uint8_t> (hasSidechain ? 1 : 0));

        sourceOffset += contiguous;
        remaining -= contiguous;
        localWritePosition = (localWritePosition + contiguous) % capacitySamples;
    }

    writePosition.store (localWritePosition);
    totalSamplesWritten.fetch_add (samplesToWrite);
}

CaptureSnapshot CaptureRingBuffer::createSnapshotLast (double seconds) const
{
    CaptureSnapshot snapshot;
    snapshot.sampleRate = currentSampleRate;
    snapshot.numChannels = channelCount;

    const auto available = getAvailableSamples();
    const auto requested = juce::jlimit (0, capacitySamples, static_cast<int> (std::ceil (seconds * currentSampleRate)));
    snapshot.numSamples = juce::jmin (available, requested);

    if (snapshot.numSamples <= 0 || channelCount <= 0)
        return snapshot;

    snapshot.main.setSize (channelCount, snapshot.numSamples);
    snapshot.sidechain.setSize (channelCount, snapshot.numSamples);

    const auto currentWritePosition = writePosition.load();
    auto sourcePosition = currentWritePosition - snapshot.numSamples;

    while (sourcePosition < 0)
        sourcePosition += capacitySamples;

    auto remaining = snapshot.numSamples;
    auto destinationOffset = 0;
    auto sidechainSamplesPresent = 0;

    while (remaining > 0)
    {
        const auto contiguous = juce::jmin (remaining, capacitySamples - sourcePosition);

        for (auto channel = 0; channel < channelCount; ++channel)
        {
            snapshot.main.copyFrom (channel, destinationOffset, mainBuffer, channel, sourcePosition, contiguous);
            snapshot.sidechain.copyFrom (channel, destinationOffset, sidechainBuffer, channel, sourcePosition, contiguous);
        }

        for (auto sample = 0; sample < contiguous; ++sample)
            sidechainSamplesPresent += sidechainMask[static_cast<size_t> (sourcePosition + sample)] != 0 ? 1 : 0;

        destinationOffset += contiguous;
        remaining -= contiguous;
        sourcePosition = (sourcePosition + contiguous) % capacitySamples;
    }

    snapshot.sidechainPresent = sidechainSamplesPresent == snapshot.numSamples;
    snapshot.mainRms = computeRms (snapshot.main);
    snapshot.sidechainRms = computeRms (snapshot.sidechain);
    snapshot.validSampleRatio = computeValidSampleRatio (snapshot.main, snapshot.sidechain);

    return snapshot;
}

int CaptureRingBuffer::getAvailableSamples() const noexcept
{
    const auto written = totalSamplesWritten.load();
    return static_cast<int> (juce::jmin<int64_t> (written, capacitySamples));
}

float CaptureRingBuffer::computeRms (const juce::AudioBuffer<float>& buffer) noexcept
{
    double sumSquares = 0.0;
    int64_t count = 0;

    for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        const auto* samples = buffer.getReadPointer (channel);

        for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            const auto value = static_cast<double> (samples[sample]);
            sumSquares += value * value;
            ++count;
        }
    }

    return count > 0 ? static_cast<float> (std::sqrt (sumSquares / static_cast<double> (count))) : 0.0f;
}

float CaptureRingBuffer::computeValidSampleRatio (const juce::AudioBuffer<float>& main,
                                                  const juce::AudioBuffer<float>& sidechain) noexcept
{
    const auto channels = juce::jmin (main.getNumChannels(), sidechain.getNumChannels());
    const auto samples = juce::jmin (main.getNumSamples(), sidechain.getNumSamples());
    int valid = 0;
    int total = 0;

    for (auto sample = 0; sample < samples; ++sample)
    {
        auto mainPeak = 0.0f;
        auto sidechainPeak = 0.0f;

        for (auto channel = 0; channel < channels; ++channel)
        {
            mainPeak = juce::jmax (mainPeak, std::abs (main.getSample (channel, sample)));
            sidechainPeak = juce::jmax (sidechainPeak, std::abs (sidechain.getSample (channel, sample)));
        }

        valid += (mainPeak > 1.0e-6f && sidechainPeak > 1.0e-6f) ? 1 : 0;
        ++total;
    }

    return total > 0 ? static_cast<float> (valid) / static_cast<float> (total) : 0.0f;
}
} // namespace RCCapture
