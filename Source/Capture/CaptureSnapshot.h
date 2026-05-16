#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace RCCapture
{
struct CaptureSnapshot
{
    double sampleRate = 0.0;
    int numChannels = 0;
    int numSamples = 0;

    juce::AudioBuffer<float> main;
    juce::AudioBuffer<float> sidechain;

    bool sidechainPresent = false;
    float mainRms = 0.0f;
    float sidechainRms = 0.0f;
    float validSampleRatio = 0.0f;
};
} // namespace RCCapture
