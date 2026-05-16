#pragma once

#include <vector>

#include <juce_core/juce_core.h>

namespace RCML
{
class ReadoutModel final
{
public:
    void prepare (int numChannels, int featureCount);
    void reset();

    float processFeatures (int channel, const float* features, int featureCount) const noexcept;

    int getFeatureCount() const noexcept { return currentFeatureCount; }

private:
    int channelCount = 0;
    int currentFeatureCount = 0;
    bool hasNonZeroWeights = false;
    std::vector<float> weights;
};
} // namespace RCML
