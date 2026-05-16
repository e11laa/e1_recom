#pragma once

#include <vector>

#include <juce_core/juce_core.h>

namespace RCML
{
class ReservoirBank final
{
public:
    void prepare (int numChannels, int size, float leak, float inputGain, float feedbackGain);
    void reset();

    void processSample (int channel, float inputSample) noexcept;

    int getSize() const noexcept { return bankSize; }
    float getState (int channel, int index) const noexcept;

private:
    static float softsign (float value) noexcept;
    static float sanitize (float value) noexcept;

    int channelCount = 0;
    int bankSize = 0;
    float leakAmount = 0.0f;
    float inputAmount = 0.0f;
    float feedbackAmount = 0.0f;

    std::vector<float> states;
    std::vector<int> writePositions;
};
} // namespace RCML
