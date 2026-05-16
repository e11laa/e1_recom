#pragma once

#include <atomic>

#include <juce_core/juce_core.h>

#include "FeatureExtractor.h"
#include "ReadoutModel.h"
#include "ReservoirConfig.h"

namespace RCML
{
class ReservoirRuntime final
{
public:
    void prepare (const ReservoirConfig& config);
    void reset();

    float processSample (int channel, float inputSample) noexcept;

    int getFeatureCount() const noexcept { return readoutModel.getFeatureCount(); }
    bool isReservoirEnabled() const noexcept { return reservoirEnabled; }
    void setReservoirEnabled (bool enabled) noexcept { reservoirEnabled = enabled; }
    float getLastReservoirPeak() const noexcept { return lastReservoirPeak; }
    int getNanDetectedCount() const noexcept { return nanDetectedCount.load(); }

private:
    FeatureExtractor featureExtractor;
    ReadoutModel readoutModel;

    bool reservoirEnabled = true;
    float lastReservoirPeak = 0.0f;
    std::atomic<int> nanDetectedCount { 0 };
};
} // namespace RCML
