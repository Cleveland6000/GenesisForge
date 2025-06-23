
#include <glad/glad.h>
#include "application.hpp"
#include <iostream>

const float Application::CLEAR_COLOR_R = 0.2f;
const float Application::CLEAR_COLOR_G = 0.3f;
const float Application::CLEAR_COLOR_B = 0.3f;
const float Application::CLEAR_COLOR_A = 1.0f;

Application::Application()
    : m_window(nullptr, glfwDestroyWindow)
{
}

Application::~Application()
{
    glfwTerminate();
}

bool Application::initialize()
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window.reset(glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Hello OpenGL", NULL, NULL));
    if (!m_window)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window.get());

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD\n";
        return false;
    }

    glfwSetFramebufferSizeCallback(m_window.get(), ::framebuffer_size_callback);
    int initialWidth, initialHeight;
    glfwGetFramebufferSize(m_window.get(), &initialWidth, &initialHeight);
    ::framebuffer_size_callback(m_window.get(), initialWidth, initialHeight);

    return true;
}

void Application::run()
{
    while (!glfwWindowShouldClose(m_window.get()))
    {
        processInput();
        update();
        render();

        glfwSwapBuffers(m_window.get());
        glfwPollEvents();
    }
}

void Application::processInput()
{
    if (glfwGetKey(m_window.get(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(m_window.get(), true);
    }
}

void Application::update()
{
    // Update logic can be added here
}

void Application::render()
{
    glClearColor(CLEAR_COLOR_R, CLEAR_COLOR_G, CLEAR_COLOR_B, CLEAR_COLOR_A);
    glClear(GL_COLOR_BUFFER_BIT);

    // Rendering code can be added here
}