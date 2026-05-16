#pragma once

#include <atomic>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>

#include "CaptureSnapshot.h"

namespace RCCapture
{
class CaptureRingBuffer final
{
public:
    void prepare (double sampleRate, double maxCaptureSeconds, int numChannels);
    void reset();

    void pushBlock (const juce::AudioBuffer<float>& main,
                    const juce::AudioBuffer<float>* sidechain,
                    int numSamples) noexcept;

    CaptureSnapshot createSnapshotLast (double seconds) const;

    int getCapacitySamples() const noexcept { return capacitySamples; }
    int getAvailableSamples() const noexcept;
    bool isEmpty() const noexcept { return getAvailableSamples() <= 0; }

private:
    static float computeRms (const juce::AudioBuffer<float>& buffer) noexcept;
    static float computeValidSampleRatio (const juce::AudioBuffer<float>& main,
                                          const juce::AudioBuffer<float>& sidechain) noexcept;

    double currentSampleRate = 0.0;
    int capacitySamples = 0;
    int channelCount = 0;

    juce::AudioBuffer<float> mainBuffer;
    juce::AudioBuffer<float> sidechainBuffer;
    std::vector<uint8_t> sidechainMask;

    std::atomic<int> writePosition { 0 };
    std::atomic<int64_t> totalSamplesWritten { 0 };
};
} // namespace RCCapture
