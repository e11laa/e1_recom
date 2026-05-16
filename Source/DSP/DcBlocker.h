#pragma once

#include <cmath>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>

namespace RCDSP
{
class DcBlocker final
{
public:
    void prepare (double newSampleRate, int numChannels)
    {
        const auto safeSampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
        const auto target = std::exp (-2.0 * juce::MathConstants<double>::pi * 20.0 / safeSampleRate);
        coefficient = static_cast<float> (juce::jlimit (0.995, 0.9999, target));

        states.resize (static_cast<size_t> (juce::jmax (0, numChannels)));
        reset();
    }

    void reset() noexcept
    {
        for (auto& state : states)
            state = {};
    }

    float processSample (int channel, float input) noexcept
    {
        if (channel < 0 || static_cast<size_t> (channel) >= states.size())
            return input;

        auto& state = states[static_cast<size_t> (channel)];
        const auto output = input - state.previousInput + coefficient * state.previousOutput;

        state.previousInput = input;
        state.previousOutput = output;

        return output;
    }

    void processBlock (juce::AudioBuffer<float>& buffer) noexcept
    {
        const auto numChannels = juce::jmin (buffer.getNumChannels(), static_cast<int> (states.size()));
        const auto numSamples = buffer.getNumSamples();

        for (auto channel = 0; channel < numChannels; ++channel)
        {
            auto* samples = buffer.getWritePointer (channel);

            for (auto sample = 0; sample < numSamples; ++sample)
                samples[sample] = processSample (channel, samples[sample]);
        }
    }

private:
    struct ChannelState
    {
        float previousInput = 0.0f;
        float previousOutput = 0.0f;
    };

    std::vector<ChannelState> states;
    float coefficient = 0.995f;
};
} // namespace RCDSP
