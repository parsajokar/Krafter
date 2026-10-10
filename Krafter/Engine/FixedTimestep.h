#pragma once

namespace Krafter {

class FixedTimestep {
public:
    FixedTimestep(float stepSeconds, int maxStepsPerFrame)
        : m_Step(stepSeconds)
        , m_MaxStepsPerFrame(maxStepsPerFrame)
    {
    }

    int Advance(float frameSeconds)
    {
        m_Accumulator += frameSeconds;
        const int steps = static_cast<int>(m_Accumulator / m_Step);
        m_Accumulator -= static_cast<float>(steps) * m_Step;
        return steps < m_MaxStepsPerFrame ? steps : m_MaxStepsPerFrame;
    }

    float GetStep() const
    {
        return m_Step;
    }

    float GetAlpha() const
    {
        return m_Accumulator / m_Step;
    }

private:
    float m_Step;
    int m_MaxStepsPerFrame;
    float m_Accumulator = 0.0f;
};

}
