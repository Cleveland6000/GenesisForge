#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <GLFW/glfw3.h>
#include <memory>
#include "opengl_utils.hpp"

class Application
{
public:
    Application();
    ~Application();
    bool initialize();
    void run();

private:
    std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> m_window;

    static const unsigned int SCR_WIDTH = 800;
    static const unsigned int SCR_HEIGHT = 600;
    static const float CLEAR_COLOR_R;
    static const float CLEAR_COLOR_G;
    static const float CLEAR_COLOR_B;
    static const float CLEAR_COLOR_A;

    unsigned int m_VAO;
    unsigned int m_VBO;
    unsigned int m_EBO;
    unsigned int m_shaderProgram;

    void processInput();
    void update();
    void render();
    
};

#endif // APPLICATION_HPP