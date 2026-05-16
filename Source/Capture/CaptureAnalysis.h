#pragma once

#include <juce_core/juce_core.h>

namespace RCCapture
{
struct CaptureAnalysis
{
    bool valid = false;
    bool sidechainPresent = false;
    bool tooQuiet = false;
    bool misaligned = false;

    float mainRmsDb = -100.0f;
    float sidechainRmsDb = -100.0f;
    float validSampleRatio = 0.0f;
    float alignmentScore = 0.0f;
    int estimatedLagSamples = 0;

    juce::String statusText { "Capture: Empty" };
};
} // namespace RCCapture
