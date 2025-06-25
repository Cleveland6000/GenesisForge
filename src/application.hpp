#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>
#include <string>
#include "camera.hpp"
#include "timer.hpp"
#include "input_manager.hpp"
#include "fullscreen_manager.hpp"
#include "chunk.hpp"
#include "FontLoader.hpp"
#include "renderer.hpp"

class Application
{
public:
    Application();
    ~Application();

    bool initialize();
    void run();
    void updateProjectionMatrix(int width, int height);

private:
    static const float CLEAR_COLOR_R, CLEAR_COLOR_G, CLEAR_COLOR_B, CLEAR_COLOR_A;
    static constexpr int SCR_WIDTH = 800, SCR_HEIGHT = 600, m_gridSize = 16;
    static constexpr float m_cubeSpacing = 1.0f;

    std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> m_window;
    Camera m_camera;
    Timer m_timer;
    std::unique_ptr<InputManager> m_inputManager;
    FullscreenManager m_fullscreenManager;
    FontLoader m_fontLoader;
    FontData m_fontData;
    std::unique_ptr<Chunk> m_chunk;
    std::unique_ptr<Renderer> m_renderer;
    std::string m_fpsString, m_positionString;
    glm::mat4 m_projectionMatrix;

    bool initGLFW(), createWindowAndContext(), initializeManagers(), initializeChunkAndFont(), initializeRenderer();
    void setupCallbacks(), processInput(), update(), render(), updateFpsAndPositionStrings();

    static void staticFramebufferSizeCallback(GLFWwindow *, int, int);
    static void staticMouseCallback(GLFWwindow *, double, double);
};

#endif // APPLICATION_HPP
