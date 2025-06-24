#ifndef TIMER_HPP
#define TIMER_HPP

class Timer
{
public:
    Timer();
    float tick();
    float getDeltaTime() const { return m_deltaTime; }
    float getTotalTime() const { return m_totalTime; }

private:
    float m_lastFrameTime;
    float m_deltaTime;
    float m_totalTime;
};

#endif // TIMER_HPP