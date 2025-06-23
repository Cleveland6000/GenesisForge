#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <GLFW/glfw3.h>
#include <memory>
#include "opengl_utils.hpp"
#include <vector> // std::vector を使用する場合

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

    bool m_projectionNeedsUpdate;

    glm::mat4 m_projectionMatrix;
    std::vector<glm::vec3> m_cubePositions;
    
    void processInput();
    void update();
    void render();

    void updateProjectionMatrix(int width, int height);

    static void staticFramebufferSizeCallback(GLFWwindow *window, int width, int height);
};

#endif // APPLICATION_HPP