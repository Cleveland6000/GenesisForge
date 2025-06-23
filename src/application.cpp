
#include <glad/glad.h>
#include "application.hpp"
#include <iostream>

const float Application::CLEAR_COLOR_R = 0.2f;
const float Application::CLEAR_COLOR_G = 0.3f;
const float Application::CLEAR_COLOR_B = 0.3f;
const float Application::CLEAR_COLOR_A = 1.0f;

Application::Application()
    : m_window(nullptr, glfwDestroyWindow),
      m_VAO(0), m_VBO(0), m_EBO(0), m_shaderProgram(0)
{
}

Application::~Application()
{

    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_VBO);
    glDeleteBuffers(1, &m_EBO);
    glDeleteProgram(m_shaderProgram);
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

    glEnable(GL_DEPTH_TEST);

    float vertices[] =
        {
            // 位置               色 (RGB)
            // 奥の面 (Z=-0.5f)
            -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, // 青
            0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f,
            0.5f, 0.5f, -0.5f, 0.0f, 0.0f, 1.0f,
            -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, 1.0f,

            // 手前の面 (Z=0.5f)
            -0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, // 赤
            0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
            0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
            -0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,

            // 左の面 (X=-0.5f)
            -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, // 緑
            -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
            -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
            -0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f,

            // 右の面 (X=0.5f)
            0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f, // 黄
            0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f,
            0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f,
            0.5f, -0.5f, 0.5f, 1.0f, 1.0f, 0.0f,

            // 上の面 (Y=0.5f)
            -0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 1.0f, // マゼンタ
            0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 1.0f,
            0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 1.0f,
            -0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 1.0f,

            // 下の面 (Y=-0.5f)
            -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 1.0f, // シアン
            0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 1.0f,
            0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 1.0f,
            -0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 1.0f};

    unsigned int indices[] =
        {
            // 奥の面
            0, 1, 2,
            2, 3, 0,
            // 手前の面
            4, 5, 6,
            6, 7, 4,
            // 左の面
            8, 9, 10,
            10, 11, 8,
            // 右の面
            12, 13, 14,
            14, 15, 12,
            // 上の面
            16, 17, 18,
            18, 19, 16,
            // 下の面
            20, 21, 22,
            22, 23, 20};

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);
    glBindVertexArray(m_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    m_shaderProgram = ::createShaderProgram("shaders/basic.vert", "shaders/basic.frag");

    if (m_shaderProgram == 0)
    {
        std::cerr << "Failed to create shader program\n";
        return false;
    }
    glm::mat4 model = glm::mat4(1.0f);                                            // 単位行列で初期化
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, -2.0f));                  // Z軸方向に-2.0f移動 (カメラから離れる)
    model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Y軸周りに45度回転

    // ビュー行列 (カメラの位置と向き)
    glm::mat4 view = glm::mat4(1.0f); // 単位行列で初期化
    // LookAt関数を使ってカメラを設定:
    // 1. カメラの位置 (0,0,0)
    // 2. 見るターゲットの中心 (0,0,0)
    // 3. 上方向のベクトル (0,1,0)
    // このビュー行列は、ワールド座標をカメラの視点から見た座標に変換します。
    // 現在、モデル行列でZ方向に動かしているので、ここではカメラは原点でOK。
    view = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f),  // カメラの位置
                       glm::vec3(0.0f, 0.0f, -1.0f), // ターゲット（Z軸マイナス方向を見る）
                       glm::vec3(0.0f, 1.0f, 0.0f)); // 上方向

    // 投影行列 (パースペクティブ投影)
    glm::mat4 projection = glm::perspective(glm::radians(45.0f),                  // 視野角 (FOV) 45度
                                            (float)SCR_WIDTH / (float)SCR_HEIGHT, // アスペクト比
                                            0.1f,                                 // near clipping plane
                                            100.0f);                              // far clipping plane

    // シェーダープログラムを有効にしてからユニフォーム変数を設定
    glUseProgram(m_shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
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
    // ここを修正: GL_DEPTH_BUFFER_BIT を追加
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(m_shaderProgram);

    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Rendering code can be added here
}