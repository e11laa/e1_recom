#include <cmath>
#include <iostream>

#include <juce_audio_basics/juce_audio_basics.h>

#include "../Source/Capture/CaptureManager.h"
#include "../Source/Capture/CaptureRingBuffer.h"

namespace
{
bool check (bool condition, const char* message)
{
    if (! condition)
        std::cerr << message << '\n';

    return condition;
}

void fillTone (juce::AudioBuffer<float>& buffer, int offsetSamples = 0)
{
    constexpr auto frequency = 440.0f;
    constexpr auto sampleRate = 48000.0f;

    for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* samples = buffer.getWritePointer (channel);

        for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            const auto phase = juce::MathConstants<float>::twoPi * frequency
                * static_cast<float> (sample + offsetSamples) / sampleRate;
            samples[sample] = 0.25f * std::sin (phase);
        }
    }
}

void fillPulseTrain (juce::AudioBuffer<float>& buffer, int delaySamples = 0)
{
    buffer.clear();

    for (auto sample = delaySamples; sample < buffer.getNumSamples(); sample += 4800)
    {
        for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
            buffer.setSample (channel, sample, 0.8f);
    }
}

void fillModulatedTone (juce::AudioBuffer<float>& buffer, int delaySamples = 0)
{
    constexpr auto carrierHz = 440.0f;
    constexpr auto sampleRate = 48000.0f;
    buffer.clear();

    for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        for (auto sample = delaySamples; sample < buffer.getNumSamples(); ++sample)
        {
            const auto sourceSample = sample - delaySamples;
            const auto t = static_cast<float> (sourceSample) / sampleRate;
            const auto envelope = 0.25f
                + 0.15f * std::sin (juce::MathConstants<float>::twoPi * 3.0f * t)
                + 0.08f * std::sin (juce::MathConstants<float>::twoPi * 7.0f * t);
            const auto carrier = std::sin (juce::MathConstants<float>::twoPi * carrierHz * t);

            buffer.setSample (channel, sample, envelope * carrier);
        }
    }
}
} // namespace

int main()
{
    bool ok = true;

    RCCapture::CaptureRingBuffer ringBuffer;
    ringBuffer.prepare (10.0, 1.0, 2);

    juce::AudioBuffer<float> mainBlock (2, 7);
    juce::AudioBuffer<float> sidechainBlock (2, 7);
    mainBlock.clear();
    sidechainBlock.clear();

    for (auto sample = 0; sample < 7; ++sample)
    {
        mainBlock.setSample (0, sample, static_cast<float> (sample + 1));
        mainBlock.setSample (1, sample, static_cast<float> (sample + 1));
        sidechainBlock.setSample (0, sample, static_cast<float> (sample + 1));
        sidechainBlock.setSample (1, sample, static_cast<float> (sample + 1));
    }

    ringBuffer.pushBlock (mainBlock, &sidechainBlock, 7);
    ringBuffer.pushBlock (mainBlock, &sidechainBlock, 7);
    auto wrapped = ringBuffer.createSnapshotLast (1.0);
    ok &= check (wrapped.numSamples == 10, "CaptureRingBuffer did not keep its wrapped capacity");
    ok &= check (wrapped.sidechainPresent, "CaptureRingBuffer lost sidechain mask across wrap");

    RCCapture::CaptureManager manager;
    manager.prepare (48000.0, 512, 2, 30.0);
    manager.setLearnArmed (true);

    juce::AudioBuffer<float> normalMain (2, 48000);
    juce::AudioBuffer<float> normalSidechain (2, 48000);
    fillTone (normalMain);
    fillTone (normalSidechain);

    manager.pushAudioBlock (normalMain, nullptr, normalMain.getNumSamples());
    auto missingSidechain = manager.analyzeSnapshot (manager.createSnapshotLast (1.0));
    ok &= check (! missingSidechain.valid && missingSidechain.statusText == "Capture: Sidechain Missing",
                 "Missing sidechain snapshot was not invalid");

    manager.reset();
    manager.prepare (48000.0, 512, 2, 30.0);
    manager.setLearnArmed (true);

    juce::AudioBuffer<float> silent (2, 48000);
    silent.clear();
    manager.pushAudioBlock (silent, &silent, silent.getNumSamples());
    auto tooQuiet = manager.analyzeSnapshot (manager.createSnapshotLast (1.0));
    ok &= check (! tooQuiet.valid && tooQuiet.tooQuiet, "Silent snapshot was not marked Too Quiet");

    manager.reset();
    manager.prepare (48000.0, 512, 2, 30.0);
    manager.setLearnArmed (true);

    juce::AudioBuffer<float> alignedMain (2, 48000);
    juce::AudioBuffer<float> alignedSidechain (2, 48000);
    fillModulatedTone (alignedMain, 0);
    fillModulatedTone (alignedSidechain, 0);
    manager.pushAudioBlock (alignedMain, &alignedSidechain, alignedMain.getNumSamples());
    auto aligned = manager.analyzeSnapshot (manager.createSnapshotLast (1.0));
    ok &= check (aligned.valid, "Aligned normal snapshot was not valid");

    manager.reset();
    manager.prepare (48000.0, 512, 2, 30.0);
    manager.setLearnArmed (true);

    juce::AudioBuffer<float> delayedMain (2, 48000);
    juce::AudioBuffer<float> delayedSidechain (2, 48000);
    fillModulatedTone (delayedMain, 7200);
    fillModulatedTone (delayedSidechain, 0);
    manager.pushAudioBlock (delayedMain, &delayedSidechain, delayedMain.getNumSamples());
    auto delayed = manager.analyzeSnapshot (manager.createSnapshotLast (1.0));
    ok &= check (delayed.misaligned && std::abs (delayed.estimatedLagSamples) >= 4096,
                 "Delayed main/sidechain pair did not report a meaningful lag");

    return ok ? 0 : 1;
}
