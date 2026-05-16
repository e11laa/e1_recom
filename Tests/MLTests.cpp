#include <cmath>
#include <iostream>

#include <juce_audio_basics/juce_audio_basics.h>

#include "../Source/ML/FeatureExtractor.h"
#include "../Source/ML/ReadoutModel.h"
#include "../Source/ML/ReservoirBank.h"
#include "../Source/ML/ReservoirRuntime.h"

namespace
{
bool check (bool condition, const char* message)
{
    if (! condition)
        std::cerr << message << '\n';

    return condition;
}

uint32_t nextRandom (uint32_t& state)
{
    state = state * 1664525u + 1013904223u;
    return state;
}

float randomBipolar (uint32_t& state)
{
    const auto value = static_cast<float> (nextRandom (state) & 0x00ffffffu) / static_cast<float> (0x00ffffffu);
    return value * 2.0f - 1.0f;
}
} // namespace

int main()
{
    bool ok = true;

    RCML::ReservoirBank bank;
    bank.prepare (2, 16, 0.5f, 0.75f, 0.25f);
    bank.processSample (0, 0.5f);
    bank.processSample (1, -0.5f);

    for (auto channel = 0; channel < 2; ++channel)
    {
        for (auto index = 0; index < bank.getSize(); ++index)
            ok &= check (std::isfinite (bank.getState (channel, index)), "ReservoirBank produced non-finite state");
    }

    bank.reset();
    ok &= check (bank.getState (0, 0) == 0.0f && bank.getState (1, 0) == 0.0f,
                 "ReservoirBank reset did not clear state");

    RCML::ReservoirConfig config;
    config.sampleRate = 48000.0;
    config.numChannels = 2;

    RCML::FeatureExtractor extractor;
    extractor.prepare (config, 0);
    const auto featureCount = extractor.getFeatureCount();
    ok &= check (featureCount == 392, "FeatureExtractor feature count changed unexpectedly");

    const auto* features = extractor.processSampleAndGetFeatures (0, 0.25f);
    ok &= check (features != nullptr, "FeatureExtractor returned null features");

    for (auto index = 0; index < featureCount; ++index)
        ok &= check (std::isfinite (features[index]), "FeatureExtractor produced non-finite feature");

    const auto featureCountAfter = extractor.getFeatureCount();
    ok &= check (featureCountAfter == featureCount, "FeatureExtractor feature count was not stable");

    RCML::ReadoutModel readout;
    readout.prepare (2, featureCount);
    ok &= check (readout.processFeatures (0, features, featureCount) == 0.0f,
                 "ReadoutModel zero weights did not return zero");

    RCML::ReservoirRuntime runtime;
    runtime.prepare (config);
    ok &= check (runtime.getFeatureCount() == featureCount, "ReservoirRuntime feature count mismatch");

    for (auto sample = 0; sample < 48000; ++sample)
    {
        const auto t = static_cast<float> (sample) / 48000.0f;
        const auto input = 0.3f * std::sin (juce::MathConstants<float>::twoPi * 220.0f * t);
        const auto output = runtime.processSample (0, input);

        ok &= check (std::isfinite (output), "ReservoirRuntime sine run produced non-finite output");
        ok &= check (output == 0.0f, "ReservoirRuntime readout should be zero in P3");
    }

    runtime.reset();

    uint32_t randomState = 0x12345678u;

    for (auto sample = 0; sample < 48000; ++sample)
    {
        const auto output = runtime.processSample (sample % 2, 0.2f * randomBipolar (randomState));
        ok &= check (std::isfinite (output), "ReservoirRuntime random run produced non-finite output");
        ok &= check (output == 0.0f, "ReservoirRuntime random readout should be zero in P3");
    }

    ok &= check (runtime.getNanDetectedCount() == 0, "ReservoirRuntime detected NaN/Inf during normal tests");

    return ok ? 0 : 1;
}
