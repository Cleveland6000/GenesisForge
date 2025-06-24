#include <glad/glad.h>
#include "application.hpp"
#include <iostream>
#include "opengl_utils.hpp"
#include "camera.hpp"
#include "timer.hpp"

const float Application::CLEAR_COLOR_R = 0.0f;
const float Application::CLEAR_COLOR_G = 0.0f;
const float Application::CLEAR_COLOR_B = 0.0f;
const float Application::CLEAR_COLOR_A = 1.0f;

Application::Application()
    : m_window(nullptr, glfwDestroyWindow),
      m_VAO(0), m_VBO(0), m_EBO(0), m_shaderProgram(0),
      m_camera(glm::vec3(0.0f, 0.0f, 3.0f)), // カメラを初期位置に設定
      m_timer()                              // Timerクラスをデフォルトコンストラクタで初期化
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
    // マウス入力コールバックを設定
    glfwSetCursorPosCallback(m_window.get(), Application::staticMouseCallback);
    // スクロール入力コールバックを設定
    glfwSetScrollCallback(m_window.get(), Application::staticScrollCallback);

    // マウスカーソルを非表示にし、中心に固定
    glfwSetInputMode(m_window.get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // 初期ビューポートと投影行列を設定
    int initialWidth, initialHeight;
    glfwGetFramebufferSize(m_window.get(), &initialWidth, &initialHeight);
    // ビューポートの更新はコールバックに任せる
    Application::staticFramebufferSizeCallback(m_window.get(), initialWidth, initialHeight);

    glEnable(GL_DEPTH_TEST);

    // --- 立方体の頂点データと色データ (共有頂点に修正) ---
    float vertices[] = {
        // 位置 (XYZ)                          色 (RGB)
        // 0: 右上奥
        0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, // 赤
        // 1: 右下奥
        0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, // 緑
        // 2: 左下奥
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, // 青
        // 3: 左上奥
        -0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, // 黄

        // 4: 右上手前
        0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 1.0f, // シアン
        // 5: 右下手前
        0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 1.0f, // マゼンタ
        // 6: 左下手前
        -0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, // 灰色
        // 7: 左上手前
        -0.5f, 0.5f, 0.5f, 0.8f, 0.2f, 0.6f // 紫
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
        5, 1, 2};

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
    m_cubePositions.push_back(glm::vec3(0.0f, 0.0f, -3.0f));  // 中央
    m_cubePositions.push_back(glm::vec3(2.0f, 0.0f, -3.0f));  // 右
    m_cubePositions.push_back(glm::vec3(-2.0f, 0.0f, -3.0f)); // 左
    m_cubePositions.push_back(glm::vec3(0.0f, 2.0f, -3.0f));  // 上
    m_cubePositions.push_back(glm::vec3(0.0f, -2.0f, -3.0f)); // 下
    // もっと追加しても良い
    m_cubePositions.push_back(glm::vec3(1.0f, 1.0f, -4.0f));
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

    static bool f11_pressed_last_frame = false;
    // Application::processInput() 内
    if (glfwGetKey(m_window.get(), GLFW_KEY_F11) == GLFW_PRESS)
    {
        if (!f11_pressed_last_frame)
        {
            // 引数から width, height を削除
            m_fullscreenManager.toggleFullscreen(m_window.get());
        }
        f11_pressed_last_frame = true;
    }
    else
    {
        f11_pressed_last_frame = false;
    }

    // --- WASDキーによる移動 (Cameraクラスに処理を委譲) ---
    // m_deltaTimeの代わりにm_timer.getDeltaTime()を使用
    if (glfwGetKey(m_window.get(), GLFW_KEY_W) == GLFW_PRESS)
    {
        m_camera.processKeyboard(FORWARD, m_timer.getDeltaTime());
    }
    if (glfwGetKey(m_window.get(), GLFW_KEY_S) == GLFW_PRESS)
    {
        m_camera.processKeyboard(BACKWARD, m_timer.getDeltaTime());
    }
    if (glfwGetKey(m_window.get(), GLFW_KEY_A) == GLFW_PRESS)
    {
        m_camera.processKeyboard(LEFT, m_timer.getDeltaTime());
    }
    if (glfwGetKey(m_window.get(), GLFW_KEY_D) == GLFW_PRESS)
    {
        m_camera.processKeyboard(RIGHT, m_timer.getDeltaTime());
    }
}

// 更新処理
void Application::update()
{
    // deltaTimeの計算をTimerクラスに任せる
    // Application::m_deltaTime は不要になるため、もし他の場所で使用している場合は m_timer.getDeltaTime() に置き換える
    m_timer.tick();

    // 必要に応じてロジックを追加（例：カメラ移動、オブジェクトのアニメーションなど）

    // FPS表示ロジックもTimerクラスのgetTotalTime()を使用
    static double lastFPSTime = 0.0;
    static int frameCount = 0;

    frameCount++;
    // m_deltaTimeのチェックはm_timerが内部で処理するため不要、またはm_timer.getDeltaTime() > 0.0で確認
    if (m_timer.getDeltaTime() > 0.0) // 念のためチェック
    {
        if (m_timer.getTotalTime() - lastFPSTime >= 1.0) // 1秒ごとに更新
        {
            double fps = (double)frameCount / (m_timer.getTotalTime() - lastFPSTime);
            std::string title = "Hello OpenGL Cubes - FPS: " + std::to_string(static_cast<int>(fps));
            glfwSetWindowTitle(m_window.get(), title.c_str());

            frameCount = 0;
            lastFPSTime = m_timer.getTotalTime();
        }
    }
}
// application.cpp のどこかに resetMouseState メソッドを追加
void Application::resetMouseState()
{
    // 現在のウィンドウの中心にマウスカーソルを再配置する
    int width, height;
    glfwGetFramebufferSize(m_window.get(), &width, &height);

    m_lastX = static_cast<float>(width) / 2.0f;
    m_lastY = static_cast<float>(height) / 2.0f;
    m_firstMouse = true; // 次のマウス入力は「最初」として扱う

    // GLFWにカーソル位置をセットする
    glfwSetCursorPos(m_window.get(), m_lastX, m_lastY);
}
// 描画処理
void Application::render()
{
    glClearColor(CLEAR_COLOR_R, CLEAR_COLOR_G, CLEAR_COLOR_B, CLEAR_COLOR_A);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(m_shaderProgram);

    // 投影行列は毎フレーム送る（リサイズ対応のため）
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(m_projectionMatrix));

    // ビュー行列をカメラから取得
    glm::mat4 view = m_camera.getViewMatrix(); // Cameraクラスからビュー行列を取得
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
        // glfwGetTime() の代わりに m_timer.getTotalTime() を使う
        float angle = m_timer.getTotalTime() * 25.0f * (i + 1);                       // インデックスによって回転速度を変える
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
    m_projectionMatrix = glm::perspective(glm::radians(m_camera.Zoom), // カメラのZoomを使用
                                          aspectRatio,                 // 新しいアスペクト比
                                          0.1f,                        // near clipping plane
                                          100.0f);                     // far clipping plane
}

// マウス移動コールバック
void Application::staticMouseCallback(GLFWwindow *window, double xposIn, double yposIn)
{
    Application *app = static_cast<Application *>(glfwGetWindowUserPointer(window));
    if (app)
    {
        float xpos = static_cast<float>(xposIn);
        float ypos = static_cast<float>(yposIn);

        if (app->m_firstMouse)
        {
            app->m_lastX = xpos;
            app->m_lastY = ypos;
            app->m_firstMouse = false;
        }

        float xoffset = xpos - app->m_lastX;
        float yoffset = app->m_lastY - ypos; // Y軸は下方向が正なので反転させる

        app->m_lastX = xpos;
        app->m_lastY = ypos;

        app->m_camera.processMouseMovement(xoffset, yoffset);
    }
}

// マウススクロールコールバック
void Application::staticScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
    Application *app = static_cast<Application *>(glfwGetWindowUserPointer(window));
    if (app)
    {
        app->m_camera.processMouseScroll(static_cast<float>(yoffset));
    }
}
