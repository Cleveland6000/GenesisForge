#ifndef TIMER_HPP
#define TIMER_HPP

#include <functional>

class Timer {
public:
    explicit Timer(std::function<double()> getTimeFunc);
    float tick();
    float getDeltaTime() const { return m_deltaTime; }
    float getTotalTime() const { return m_totalTime; }
private:
    std::function<double()> m_getTimeFunc;
    float m_lastFrameTime, m_deltaTime, m_totalTime;
};

#endif // TIMER_HPP
