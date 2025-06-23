#include <glad/glad.h>
#include "application.hpp"
#include <iostream>
#include "opengl_utils.hpp" // createShaderProgram関数があるヘッダー

// 静的メンバ変数の初期化
const float Application::CLEAR_COLOR_R = 0.2f;
const float Application::CLEAR_COLOR_G = 0.3f;
const float Application::CLEAR_COLOR_B = 0.3f;
const float Application::CLEAR_COLOR_A = 1.0f;

// コンストラクタ
Application::Application()
    : m_window(nullptr, glfwDestroyWindow),
      m_VAO(0), m_VBO(0), m_EBO(0), m_shaderProgram(0)
{
}

// デストラクタ
Application::~Application()
{
    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_VBO);
    glDeleteBuffers(1, &m_EBO);
    glDeleteProgram(m_shaderProgram);
    glfwTerminate();
}

// 初期化関数
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

    m_window.reset(glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Hello OpenGL Cubes", NULL, NULL)); // ウィンドウタイトル変更
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

    // WindowUserPointerにApplicationインスタンスを関連付け、コールバック内で利用する
    glfwSetWindowUserPointer(m_window.get(), this);
    // 新しい静的コールバック関数を設定
    glfwSetFramebufferSizeCallback(m_window.get(), Application::staticFramebufferSizeCallback);

    // 初期ビューポートと投影行列を設定
    int initialWidth, initialHeight;
    glfwGetFramebufferSize(m_window.get(), &initialWidth, &initialHeight);
    // ビューポートの更新はコールバックに任せる
    Application::staticFramebufferSizeCallback(m_window.get(), initialWidth, initialHeight);

    glEnable(GL_DEPTH_TEST);

    // --- 立方体の頂点データと色データ (共有頂点に修正) ---
    float vertices[] = {
        // 位置 (XYZ)         色 (RGB)
        // 0: 右上奥
         0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f, // 赤
        // 1: 右下奥
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f, // 緑
        // 2: 左下奥
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f, // 青
        // 3: 左上奥
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f, // 黄

        // 4: 右上手前
         0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 1.0f, // シアン
        // 5: 右下手前
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f, // マゼンタ
        // 6: 左下手前
        -0.5f, -0.5f,  0.5f,  0.5f, 0.5f, 0.5f, // 灰色
        // 7: 左上手前
        -0.5f,  0.5f,  0.5f,  0.8f, 0.2f, 0.6f  // 紫
    };

    // インデックスデータ (修正)
    unsigned int indices[] = {
        // 奥の面 (2, 1, 0, 3)
        2, 1, 0,
        0, 3, 2,

        // 手前の面 (6, 5, 4, 7)
        6, 5, 4,
        4, 7, 6,

        // 左の面 (7, 3, 2, 6)
        7, 3, 2,
        2, 6, 7,

        // 右の面 (0, 1, 5, 4)
        0, 1, 5,
        5, 4, 0,

        // 上の面 (3, 0, 4, 7)
        3, 0, 4,
        4, 7, 3,

        // 下の面 (2, 6, 5, 1)
        2, 6, 5,
        5, 1, 2
    };

    // VAO, VBO, EBO生成とデータ転送
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);
    glBindVertexArray(m_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // 頂点属性ポインタを設定 (位置と色)
    // 位置 (layout = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    // 色 (layout = 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // シェーダープログラムの作成
    m_shaderProgram = ::createShaderProgram("../shaders/basic.vert", "../shaders/basic.frag");
    if (m_shaderProgram == 0)
    {
        std::cerr << "Failed to create shader program\n";
        return false;
    }

    // --- 複数の立方体の位置を初期化 ---
    m_cubePositions.push_back(glm::vec3( 0.0f,  0.0f, -3.0f)); // 中央
    m_cubePositions.push_back(glm::vec3( 2.0f,  0.0f, -3.0f)); // 右
    m_cubePositions.push_back(glm::vec3(-2.0f,  0.0f, -3.0f)); // 左
    m_cubePositions.push_back(glm::vec3( 0.0f,  2.0f, -3.0f)); // 上
    m_cubePositions.push_back(glm::vec3( 0.0f, -2.0f, -3.0f)); // 下
    // もっと追加しても良い
    m_cubePositions.push_back(glm::vec3( 1.0f,  1.0f, -4.0f));
    m_cubePositions.push_back(glm::vec3(-1.0f, -1.0f, -2.0f));


    return true;
}

// メインループ
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

// 入力処理
void Application::processInput()
{
    if (glfwGetKey(m_window.get(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(m_window.get(), true);
    }
}

// 更新処理
void Application::update()
{
    // 必要に応じてロジックを追加（例：カメラ移動、オブジェクトのアニメーションなど）
}

// 描画処理
void Application::render()
{
    glClearColor(CLEAR_COLOR_R, CLEAR_COLOR_G, CLEAR_COLOR_B, CLEAR_COLOR_A);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(m_shaderProgram);

    // 投影行列は毎フレーム送る（リサイズ対応のため）
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(m_projectionMatrix));

    // ビュー行列 (現時点では固定だが、カメラ移動を実装するならここを更新)
    glm::mat4 view = glm::mat4(1.0f);
    view = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f),  // カメラの位置
                       glm::vec3(0.0f, 0.0f, -1.0f), // 注視点
                       glm::vec3(0.0f, 1.0f, 0.0f)); // アップベクトル
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

    glBindVertexArray(m_VAO); // 一度バインドすればOK

    // 各立方体を描画
    for (size_t i = 0; i < m_cubePositions.size(); ++i)
    {
        glm::mat4 model = glm::mat4(1.0f);
        // 立方体の位置を適用
        model = glm::translate(model, m_cubePositions[i]);

        // (オプション) 各立方体に異なる回転を適用する例
        // 時間経過 + インデックスによるオフセットで個別の回転アニメーション
        float angle = (float)glfwGetTime() * 25.0f * (i + 1); // インデックスによって回転速度を変える
        model = glm::rotate(model, glm::radians(angle), glm::vec3(0.5f, 1.0f, 0.0f)); // Y軸とX軸の間で回転

        // モデル行列をシェーダーに送る
        glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

        // 立方体を描画
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }

    glBindVertexArray(0); // 描画終了後にVAOをアンバインド
}

// ウィンドウリサイズコールバック関連関数
void Application::staticFramebufferSizeCallback(GLFWwindow *window, int width, int height)
{
    Application *app = static_cast<Application *>(glfwGetWindowUserPointer(window));
    if (app)
    {
        app->updateProjectionMatrix(width, height);
    }
    glViewport(0, 0, width, height);
}

void Application::updateProjectionMatrix(int width, int height)
{
    if (width == 0 || height == 0)
    {
        return;
    }
    float aspectRatio = (float)width / (float)height;
    m_projectionMatrix = glm::perspective(glm::radians(45.0f), // 視野角 (FOV) 45度
                                          aspectRatio,           // 新しいアスペクト比
                                          0.1f,                  // near clipping plane
                                          100.0f);               // far clipping plane
}