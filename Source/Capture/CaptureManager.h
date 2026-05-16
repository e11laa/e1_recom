#pragma once

#include <atomic>

#include <juce_audio_basics/juce_audio_basics.h>

#include "CaptureAnalysis.h"
#include "CaptureRingBuffer.h"

namespace RCCapture
{
class CaptureManager final
{
public:
    void prepare (double sampleRate, int maxBlockSize, int numChannels, double maxSeconds);
    void reset();

    void setLearnArmed (bool armed) noexcept;
    bool isLearnArmed() const noexcept;
    bool hasCapturedAudio() const noexcept;

    void pushAudioBlock (const juce::AudioBuffer<float>& main,
                         const juce::AudioBuffer<float>* sidechain,
                         int numSamples) noexcept;

    CaptureSnapshot createSnapshotLast (double seconds) const;
    CaptureAnalysis analyzeSnapshot (const CaptureSnapshot& snapshot) const;

private:
    static float gainToDb (float value) noexcept;
    static float computeAlignmentScore (const CaptureSnapshot& snapshot, int& estimatedLagSamples);

    CaptureRingBuffer ringBuffer;
    std::atomic<bool> learnArmed { false };
    double currentSampleRate = 0.0;
    int currentMaxBlockSize = 0;
};
} // namespace RCCapture
