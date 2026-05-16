#include "CaptureManager.h"

#include <cmath>
#include <vector>

namespace RCCapture
{
void CaptureManager::prepare (double sampleRate, int maxBlockSize, int numChannels, double maxSeconds)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    currentMaxBlockSize = juce::jmax (1, maxBlockSize);
    ringBuffer.prepare (currentSampleRate, maxSeconds, numChannels);
}

void CaptureManager::reset()
{
    ringBuffer.reset();
    learnArmed.store (false);
}

void CaptureManager::setLearnArmed (bool armed) noexcept
{
    learnArmed.store (armed);
}

bool CaptureManager::isLearnArmed() const noexcept
{
    return learnArmed.load();
}

bool CaptureManager::hasCapturedAudio() const noexcept
{
    return ! ringBuffer.isEmpty();
}

void CaptureManager::pushAudioBlock (const juce::AudioBuffer<float>& main,
                                     const juce::AudioBuffer<float>* sidechain,
                                     int numSamples) noexcept
{
    if (! learnArmed.load())
        return;

    ringBuffer.pushBlock (main, sidechain, numSamples);
}

CaptureSnapshot CaptureManager::createSnapshotLast (double seconds) const
{
    return ringBuffer.createSnapshotLast (seconds);
}

CaptureAnalysis CaptureManager::analyzeSnapshot (const CaptureSnapshot& snapshot) const
{
    CaptureAnalysis analysis;

    if (snapshot.numSamples <= 0 || snapshot.numChannels <= 0)
    {
        analysis.statusText = "Capture: Empty";
        return analysis;
    }

    analysis.sidechainPresent = snapshot.sidechainPresent;
    analysis.mainRmsDb = gainToDb (snapshot.mainRms);
    analysis.sidechainRmsDb = gainToDb (snapshot.sidechainRms);
    analysis.validSampleRatio = snapshot.validSampleRatio;

    if (! analysis.sidechainPresent)
    {
        analysis.statusText = "Capture: Sidechain Missing";
        return analysis;
    }

    analysis.tooQuiet = analysis.mainRmsDb < -60.0f
        || analysis.sidechainRmsDb < -60.0f
        || analysis.validSampleRatio < 0.05f;

    if (analysis.tooQuiet)
    {
        analysis.statusText = "Capture: Too Quiet";
        return analysis;
    }

    analysis.alignmentScore = computeAlignmentScore (snapshot, analysis.estimatedLagSamples);

    const auto lagMs = snapshot.sampleRate > 0.0
        ? (1000.0 * static_cast<double> (std::abs (analysis.estimatedLagSamples)) / snapshot.sampleRate)
        : 0.0;

    analysis.misaligned = lagMs >= 100.0 || analysis.alignmentScore < 0.10f;

    if (analysis.misaligned)
    {
        analysis.statusText = "Capture: Misaligned";
        return analysis;
    }

    analysis.valid = true;
    analysis.statusText = "Capture: Ready";
    return analysis;
}

float CaptureManager::gainToDb (float value) noexcept
{
    return value > 0.0f ? juce::Decibels::gainToDecibels (value, -100.0f) : -100.0f;
}

float CaptureManager::computeAlignmentScore (const CaptureSnapshot& snapshot, int& estimatedLagSamples)
{
    estimatedLagSamples = 0;

    if (snapshot.sampleRate <= 0.0 || snapshot.numSamples <= 0 || snapshot.numChannels <= 0)
        return 0.0f;

    constexpr auto hopSize = 1024;
    const auto frameCount = snapshot.numSamples / hopSize;

    if (frameCount < 8)
        return 1.0f;

    std::vector<float> mainEnergy (static_cast<size_t> (frameCount), 0.0f);
    std::vector<float> sidechainEnergy (static_cast<size_t> (frameCount), 0.0f);

    for (auto frame = 0; frame < frameCount; ++frame)
    {
        const auto startSample = frame * hopSize;
        double mainSum = 0.0;
        double sidechainSum = 0.0;

        for (auto sample = 0; sample < hopSize; ++sample)
        {
            auto mainMono = 0.0f;
            auto sidechainMono = 0.0f;

            for (auto channel = 0; channel < snapshot.numChannels; ++channel)
            {
                mainMono += snapshot.main.getSample (channel, startSample + sample);
                sidechainMono += snapshot.sidechain.getSample (channel, startSample + sample);
            }

            mainMono /= static_cast<float> (snapshot.numChannels);
            sidechainMono /= static_cast<float> (snapshot.numChannels);
            mainSum += static_cast<double> (mainMono) * static_cast<double> (mainMono);
            sidechainSum += static_cast<double> (sidechainMono) * static_cast<double> (sidechainMono);
        }

        mainEnergy[static_cast<size_t> (frame)] = static_cast<float> (std::sqrt (mainSum / hopSize));
        sidechainEnergy[static_cast<size_t> (frame)] = static_cast<float> (std::sqrt (sidechainSum / hopSize));
    }

    const auto maxLagFrames = juce::jmax (1, static_cast<int> (std::ceil (snapshot.sampleRate * 0.2 / hopSize)));
    auto bestScore = -1.0f;
    auto bestLagFrames = 0;

    for (auto lag = -maxLagFrames; lag <= maxLagFrames; ++lag)
    {
        double sumMain = 0.0;
        double sumSidechain = 0.0;
        auto count = 0;

        for (auto frame = 0; frame < frameCount; ++frame)
        {
            const auto sidechainFrame = frame + lag;

            if (sidechainFrame < 0 || sidechainFrame >= frameCount)
                continue;

            sumMain += mainEnergy[static_cast<size_t> (frame)];
            sumSidechain += sidechainEnergy[static_cast<size_t> (sidechainFrame)];
            ++count;
        }

        if (count < 4)
            continue;

        const auto meanMain = sumMain / count;
        const auto meanSidechain = sumSidechain / count;
        double numerator = 0.0;
        double denominatorMain = 0.0;
        double denominatorSidechain = 0.0;

        for (auto frame = 0; frame < frameCount; ++frame)
        {
            const auto sidechainFrame = frame + lag;

            if (sidechainFrame < 0 || sidechainFrame >= frameCount)
                continue;

            const auto a = static_cast<double> (mainEnergy[static_cast<size_t> (frame)]) - meanMain;
            const auto b = static_cast<double> (sidechainEnergy[static_cast<size_t> (sidechainFrame)]) - meanSidechain;

            numerator += a * b;
            denominatorMain += a * a;
            denominatorSidechain += b * b;
        }

        const auto denominator = std::sqrt (denominatorMain * denominatorSidechain);
        const auto score = denominator > 1.0e-12 ? static_cast<float> (numerator / denominator) : 0.0f;

        if (score > bestScore)
        {
            bestScore = score;
            bestLagFrames = lag;
        }
    }

    estimatedLagSamples = bestLagFrames * hopSize;
    return juce::jlimit (0.0f, 1.0f, bestScore);
}
} // namespace RCCapture
