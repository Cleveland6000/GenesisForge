#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>
#include <memory>

#include "camera.hpp" // Cameraクラスをインクルード

// ウィンドウの初期サイズ
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

class Application {
public:
    Application();
    ~Application();

    bool initialize();
    void run();

private:
    // 静的コールバック関数
    static void staticFramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void staticMouseCallback(GLFWwindow* window, double xpos, double ypos); // 追加
    static void staticScrollCallback(GLFWwindow* window, double xoffset, double yoffset); // 追加

    // Applicationクラス内のメソッド
    void processInput();
    void update();
    void render();
    void updateProjectionMatrix(int width, int height);
    void toggleFullscreen();

    // ウィンドウ関連
    std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> m_window;
    int m_windowedX, m_windowedY, m_windowedWidth, m_windowedHeight;
    bool m_isFullscreen = false;

    // OpenGLオブジェクトID
    unsigned int m_VAO, m_VBO, m_EBO;
    unsigned int m_shaderProgram;

    // プロジェクション行列
    glm::mat4 m_projectionMatrix;

    // --- カメラ関連のメンバ変数をCameraクラスのインスタンスに置き換え ---
    Camera m_camera;

    // キューブの位置
    std::vector<glm::vec3> m_cubePositions;

    // 時間計算用
    float m_deltaTime = 0.0f;
    float m_lastFrame = 0.0f;

    // マウス入力用
    bool m_firstMouse = true;
    float m_lastX = SCR_WIDTH / 2.0f;
    float m_lastY = SCR_HEIGHT / 2.0f;

    // クリアカラー
    static const float CLEAR_COLOR_R;
    static const float CLEAR_COLOR_G;
    static const float CLEAR_COLOR_B;
    static const float CLEAR_COLOR_A;
};

#endif