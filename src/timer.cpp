#include <GLFW/glfw3.h>
#include "timer.hpp"

Timer::Timer()
    : m_lastFrameTime(0.0f), m_deltaTime(0.0f), m_totalTime(0.0f)
{
    m_lastFrameTime = static_cast<float>(glfwGetTime());
}

float Timer::tick()
{
    float currentFrameTime = static_cast<float>(glfwGetTime());
    m_deltaTime = currentFrameTime - m_lastFrameTime;
    m_totalTime += m_deltaTime;
    m_lastFrameTime = currentFrameTime;
    return m_deltaTime;
}