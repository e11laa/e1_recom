#pragma once

#include <array>

namespace RCML
{
struct ReservoirConfig
{
    double sampleRate = 44100.0;
    int numChannels = 2;

    int shortBankSize = 128;
    int midBankSize = 128;
    int slowBankSize = 128;

    float shortLeak = 0.65f;
    float midLeak = 0.25f;
    float slowLeak = 0.05f;

    float inputGain = 0.75f;
    float feedbackGain = 0.35f;
    float nonlinearity = 1.0f;

    bool includeEnvelopeFeatures = true;
    bool includeInputFeatures = true;

    int inputDelayLineSamples = 4096;
    std::array<float, 4> inputDelayTapMs { 1.0f, 5.0f, 20.0f, 50.0f };
    float fastEnvelopeMs = 5.0f;
    float slowEnvelopeMs = 120.0f;
};
} // namespace RCML
