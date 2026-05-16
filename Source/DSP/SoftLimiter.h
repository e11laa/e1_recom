#pragma once

#include <cmath>

#include <juce_audio_basics/juce_audio_basics.h>

namespace RCDSP
{
class SoftLimiter final
{
public:
    void reset() noexcept {}

    float processSample (float input) const noexcept
    {
        if (! std::isfinite (input))
            return 0.0f;

        constexpr auto threshold = 0.95f;
        constexpr auto headroom = 1.0f - threshold;
        const auto magnitude = std::abs (input);

        if (magnitude <= threshold)
            return input;

        const auto excess = magnitude - threshold;
        const auto limited = threshold + headroom * (1.0f - std::exp (-excess / headroom));

        return std::copysign (juce::jmin (limited, 1.0f), input);
    }

    void processBlock (juce::AudioBuffer<float>& buffer) const noexcept
    {
        const auto numChannels = buffer.getNumChannels();
        const auto numSamples = buffer.getNumSamples();

        for (auto channel = 0; channel < numChannels; ++channel)
        {
            auto* samples = buffer.getWritePointer (channel);

            for (auto sample = 0; sample < numSamples; ++sample)
                samples[sample] = processSample (samples[sample]);
        }
    }
};
} // namespace RCDSP
