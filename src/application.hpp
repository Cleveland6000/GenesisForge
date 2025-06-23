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

    bool m_isFullscreen = false; // 現在フルスクリーンモードかどうか
    int m_windowedX;             // ウィンドウモード時のX座標
    int m_windowedY;             // ウィンドウモード時のY座標
    int m_windowedWidth;         // ウィンドウモード時の幅
    int m_windowedHeight;        // ウィンドウモード時の高さ

    glm::vec3 m_cameraPos;
    glm::vec3 m_cameraFront;
    glm::vec3 m_cameraUp;
    float m_cameraSpeed;

    float m_deltaTime = 0.0f; // 前フレームからの時間差
    float m_lastFrame = 0.0f; // 前フレームの時間

    void processInput();
    void update();
    void render();

    void updateProjectionMatrix(int width, int height);
    void toggleFullscreen();
    static void staticFramebufferSizeCallback(GLFWwindow *window, int width, int height);
};

#endif // APPLICATION_HPP