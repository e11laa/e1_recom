#pragma once

#include <atomic>

#include <juce_audio_basics/juce_audio_basics.h>

#include "../ML/ReservoirRuntime.h"
#include "DcBlocker.h"
#include "SoftLimiter.h"

namespace RCDSP
{
class AudioEngine final
{
public:
    void prepare (double sampleRate, int maxBlockSize, int numChannels);
    void reset();

    void setBypass (bool enabled) noexcept;
    void setInputGainDb (float db) noexcept;
    void setOutputGainDb (float db) noexcept;

    void processBlock (juce::AudioBuffer<float>& mainBuffer,
                       const juce::AudioBuffer<float>* sidechainBuffer) noexcept;

    bool getSidechainPresent() const noexcept;
    int getReservoirFeatureCount() const noexcept;
    bool isReservoirEnabled() const noexcept;
    float getLastReservoirPeak() const noexcept;
    int getReservoirNanDetectedCount() const noexcept;

private:
    static float dbToGain (float db) noexcept;
    void updateSidechainState (const juce::AudioBuffer<float>* sidechainBuffer) noexcept;

    DcBlocker dcBlocker;
    SoftLimiter softLimiter;
    RCML::ReservoirRuntime reservoirRuntime;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> inputGain;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outputGain;

    std::atomic<bool> sidechainPresent { false };
    bool bypassed = false;
    int preparedChannels = 0;
    int preparedMaxBlockSize = 0;
};
} // namespace RCDSP
