#include "timer.hpp"

Timer::Timer(std::function<double()> getTimeFunc)
    : m_getTimeFunc(getTimeFunc),
      m_lastFrameTime(static_cast<float>(getTimeFunc())),
      m_deltaTime(0.0f),
      m_totalTime(0.0f) {}

float Timer::tick() {
    float current = static_cast<float>(m_getTimeFunc());
    m_deltaTime = current - m_lastFrameTime;
    m_totalTime += m_deltaTime;
    m_lastFrameTime = current;
    return m_deltaTime;
}
