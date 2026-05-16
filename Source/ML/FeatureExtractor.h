#pragma once

#include <vector>

#include <juce_core/juce_core.h>

#include "ReservoirBank.h"
#include "ReservoirConfig.h"

namespace RCML
{
class FeatureExtractor final
{
public:
    void prepare (const ReservoirConfig& config, int maxFeatureCount);
    void reset();

    int getFeatureCount() const noexcept { return featureCount; }

    const float* processSampleAndGetFeatures (int channel, float inputSample) noexcept;

private:
    static float sanitize (float value) noexcept;
    static float calculateEnvelopeCoefficient (double sampleRate, float timeMs) noexcept;

    void fillBankFeatures (float* destination, int& featureIndex, const ReservoirBank& bank, int channel) const noexcept;
    float readDelayedInput (int channel, int delaySamples) const noexcept;

    ReservoirConfig currentConfig;
    ReservoirBank shortBank;
    ReservoirBank midBank;
    ReservoirBank slowBank;

    int channelCount = 0;
    int featureCount = 0;
    int inputDelayLineSize = 0;
    float fastEnvelopeCoefficient = 0.0f;
    float slowEnvelopeCoefficient = 0.0f;

    std::vector<int> delayTapSamples;
    std::vector<float> inputDelayLines;
    std::vector<int> inputDelayWritePositions;
    std::vector<float> fastEnvelopes;
    std::vector<float> slowEnvelopes;
    std::vector<float> featureBuffers;
};
} // namespace RCML
