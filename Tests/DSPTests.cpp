#include <cmath>
#include <iostream>
#include <limits>

#include <juce_audio_basics/juce_audio_basics.h>

#include "../Source/DSP/AudioEngine.h"
#include "../Source/DSP/DcBlocker.h"
#include "../Source/DSP/SoftLimiter.h"

namespace
{
bool check (bool condition, const char* message)
{
    if (! condition)
        std::cerr << message << '\n';

    return condition;
}
} // namespace

int main()
{
    bool ok = true;

    RCDSP::DcBlocker dcBlocker;
    dcBlocker.prepare (48000.0, 1);
    const auto first = dcBlocker.processSample (0, 1.0f);
    dcBlocker.reset();
    const auto afterReset = dcBlocker.processSample (0, 1.0f);
    ok &= check (first == afterReset, "DcBlocker reset did not restore initial state");

    RCDSP::SoftLimiter limiter;
    for (auto value : { -1000.0f, -2.0f, -0.5f, 0.0f, 0.5f, 2.0f, 1000.0f })
        ok &= check (std::isfinite (limiter.processSample (value)), "SoftLimiter produced a non-finite value");

    ok &= check (limiter.processSample (std::numeric_limits<float>::quiet_NaN()) == 0.0f,
                 "SoftLimiter did not sanitize NaN input");

    RCDSP::AudioEngine engine;
    engine.prepare (48000.0, 64, 2);
    engine.setBypass (false);
    engine.setInputGainDb (0.0f);
    engine.setOutputGainDb (0.0f);

    juce::AudioBuffer<float> mainBuffer (2, 64);
    mainBuffer.clear();
    mainBuffer.setSample (0, 0, 0.25f);
    mainBuffer.setSample (1, 0, -0.25f);
    engine.processBlock (mainBuffer, nullptr);

    for (auto channel = 0; channel < mainBuffer.getNumChannels(); ++channel)
    {
        for (auto sample = 0; sample < mainBuffer.getNumSamples(); ++sample)
            ok &= check (std::isfinite (mainBuffer.getSample (channel, sample)),
                         "AudioEngine produced a non-finite value");
    }

    ok &= check (! engine.getSidechainPresent(), "AudioEngine reported missing sidechain as present");

    engine.setBypass (true);
    mainBuffer.setSample (0, 0, 0.125f);
    mainBuffer.setSample (1, 0, -0.125f);
    engine.processBlock (mainBuffer, nullptr);
    ok &= check (mainBuffer.getSample (0, 0) == 0.125f && mainBuffer.getSample (1, 0) == -0.125f,
                 "AudioEngine bypass did not leave the main buffer untouched");
    engine.setBypass (false);

    juce::AudioBuffer<float> sidechainBuffer (1, 64);
    sidechainBuffer.clear();
    engine.processBlock (mainBuffer, &sidechainBuffer);
    ok &= check (engine.getSidechainPresent(), "AudioEngine did not report sidechain buffer as present");

    return ok ? 0 : 1;
}
