#include "ReadoutModel.h"

#include <cmath>

namespace RCML
{
void ReadoutModel::prepare (int numChannels, int featureCount)
{
    channelCount = juce::jmax (0, numChannels);
    currentFeatureCount = juce::jmax (0, featureCount);
    weights.resize (static_cast<size_t> (channelCount * currentFeatureCount));
    reset();
}

void ReadoutModel::reset()
{
    std::fill (weights.begin(), weights.end(), 0.0f);
    hasNonZeroWeights = false;
}

float ReadoutModel::processFeatures (int channel, const float* features, int featureCount) const noexcept
{
    if (channel < 0 || channel >= channelCount || features == nullptr || featureCount != currentFeatureCount)
        return 0.0f;

    if (! hasNonZeroWeights)
        return 0.0f;

    const auto* channelWeights = weights.data() + static_cast<size_t> (channel * currentFeatureCount);
    double output = 0.0;

    for (auto i = 0; i < currentFeatureCount; ++i)
        output += static_cast<double> (features[i]) * static_cast<double> (channelWeights[i]);

    return std::isfinite (output) ? juce::jlimit (-8.0f, 8.0f, static_cast<float> (output)) : 0.0f;
}
} // namespace RCML
