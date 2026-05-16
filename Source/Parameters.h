#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace RCParameters
{
inline constexpr auto bypassId = "bypass";
inline constexpr auto inputGainDbId = "inputGainDb";
inline constexpr auto outputGainDbId = "outputGainDb";
inline constexpr auto learnArmedId = "learnArmed";

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { bypassId, 1 },
        "Bypass",
        false));

    const juce::NormalisableRange<float> gainRange { -24.0f, 24.0f, 0.01f };

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { inputGainDbId, 1 },
        "Input Gain",
        gainRange,
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { outputGainDbId, 1 },
        "Output Gain",
        gainRange,
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { learnArmedId, 1 },
        "Learn Armed",
        false));

    return layout;
}
} // namespace RCParameters
