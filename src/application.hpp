#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "camera.hpp"
#include "fullscreen_manager.hpp"

// ウィンドウの初期サイズ
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

class Application
{
public:
    Application();
    ~Application();

    bool initialize();
    void run();

    void updateProjectionMatrix(int width, int height);

    // 【追加】マウス状態をリセットするパブリックメソッド
    void resetMouseState();

private:
    // クリアカラーの静的定数
    static const float CLEAR_COLOR_R;
    static const float CLEAR_COLOR_G;
    static const float CLEAR_COLOR_B;
    static const float CLEAR_COLOR_A;

    std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> m_window;

    unsigned int m_VAO, m_VBO, m_EBO;
    unsigned int m_shaderProgram;

    // カメラ
    Camera m_camera;

    // 時間管理
    float m_lastFrame = 0.0f;
    float m_deltaTime = 0.0f;

    // 投影行列
    glm::mat4 m_projectionMatrix;

    // マウス入力管理
    bool m_firstMouse = true;
    float m_lastX = SCR_WIDTH / 2.0f;
    float m_lastY = SCR_HEIGHT / 2.0f;

    // フルスクリーンマネージャーのインスタンス
    FullscreenManager m_fullscreenManager;

    // 複数の立方体の位置
    std::vector<glm::vec3> m_cubePositions;

    void processInput();
    void update();
    void render();

    // コールバック関数
    static void staticFramebufferSizeCallback(GLFWwindow *window, int width, int height);
    static void staticMouseCallback(GLFWwindow *window, double xposIn, double yposIn);
    static void staticScrollCallback(GLFWwindow *window, double xoffset, double yoffset);
};

#endif // APPLICATION_HPP