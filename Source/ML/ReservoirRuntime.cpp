#include "ReservoirRuntime.h"

#include <cmath>

namespace RCML
{
void ReservoirRuntime::prepare (const ReservoirConfig& config)
{
    featureExtractor.prepare (config, 0);
    readoutModel.prepare (config.numChannels, featureExtractor.getFeatureCount());
    reset();
}

void ReservoirRuntime::reset()
{
    featureExtractor.reset();
    readoutModel.reset();
    lastReservoirPeak = 0.0f;
    nanDetectedCount.store (0);
}

float ReservoirRuntime::processSample (int channel, float inputSample) noexcept
{
    if (! reservoirEnabled)
        return 0.0f;

    const auto* features = featureExtractor.processSampleAndGetFeatures (channel, inputSample);
    auto output = readoutModel.processFeatures (channel, features, featureExtractor.getFeatureCount());

    if (! std::isfinite (output))
    {
        nanDetectedCount.fetch_add (1);
        output = 0.0f;
    }

    lastReservoirPeak = juce::jmax (lastReservoirPeak * 0.9995f, std::abs (output));
    return output;
}
} // namespace RCML
