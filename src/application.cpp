#include <glad/glad.h>
#include "application.hpp" // application.hpp をインクルード
#include <iostream>
#include <iomanip> // std::fixed, std::setprecision のために必要
#include <sstream> // std::stringstream のために必要
// <random> はChunkクラスに移動したので、ここでは不要
#include "opengl_utils.hpp"  // createShaderProgram関数があるヘッダー
#include "camera.hpp"        // Cameraクラスをインクルード
#include "timer.hpp"         // Timerクラスをインクルード
#include "FontLoader.hpp"    // FontLoaderをインクルード
#include "TextRenderer.hpp"  // TextRendererをインクルード
#include "input_manager.hpp" // InputManagerをインクルude
#include "chunk.hpp"         // Chunkクラスの定義をインクルード

// 静的メンバ変数の初期化
const float Application::CLEAR_COLOR_R = 0.0f;
const float Application::CLEAR_COLOR_G = 0.0f;
const float Application::CLEAR_COLOR_B = 0.0f;
const float Application::CLEAR_COLOR_A = 1.0f;

// コンストラクタ
Application::Application()
    : m_window(nullptr, glfwDestroyWindow),
      m_VAO(0), m_VBO(0), m_EBO(0), m_shaderProgram(0),
      m_camera(glm::vec3(0.0f, 0.0f, 0.0f)), // カメラを初期位置に設定
      m_timer(),                             // Timerクラスをデフォルトコンストラクタで初期化
      m_fontLoader(),                        // FontLoaderを初期化
      m_textRenderer(),                      // TextRendererを初期化
      m_inputManager(nullptr),               // InputManagerはinitializeで初期化
      m_chunk(nullptr)                       // Chunkはinitializeで初期化
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
    // プライマリモニタのビデオモードを取得
    GLFWmonitor *primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(primaryMonitor);

    // ウィンドウを中央に配置するための座標を計算
    int windowPosX = (mode->width - SCR_WIDTH) / 2;
    int windowPosY = (mode->height - SCR_HEIGHT) / 2;

    // ウィンドウの位置を設定
    glfwSetWindowPos(m_window.get(), windowPosX, windowPosY);

    glfwMakeContextCurrent(m_window.get());
    // V-Syncを無効化（ティアリングが発生する可能性がありますが、カクつきの原因特定に役立ちます）
    glfwSwapInterval(0);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD\n";
        return false;
    }

    // WindowUserPointerにApplicationインスタンスを関連付け、コールバック内で利用する
    glfwSetWindowUserPointer(m_window.get(), this);
    // フレームバッファサイズ変更コールバックを設定
    glfwSetFramebufferSizeCallback(m_window.get(), Application::staticFramebufferSizeCallback);
    // マウス入力コールバックを設定
    glfwSetCursorPosCallback(m_window.get(), Application::staticMouseCallback);

    // InputManagerを初期化
    m_inputManager = std::make_unique<InputManager>(m_window.get(), m_camera);

    // FullscreenManagerにコールバックを設定
    m_fullscreenManager.setWindowSizeChangeCallback([](int width, int int_height) { // 'height' を 'int_height' に変更して警告を回避
        GLFWwindow *currentWindow = glfwGetCurrentContext();
        if (currentWindow)
        {
            Application *app = static_cast<Application *>(glfwGetWindowUserPointer(currentWindow));
            if (app)
            {
                app->updateProjectionMatrix(width, int_height);
            }
        }
    });

    m_fullscreenManager.setMouseResetCallback([]()
                                              {
        GLFWwindow* currentWindow = glfwGetCurrentContext();
        if (currentWindow) {
            Application* app = static_cast<Application*>(glfwGetWindowUserPointer(currentWindow));
            if (app && app->m_inputManager) { 
                app->m_inputManager->resetMouseState();
            }
        } });

    m_fullscreenManager.toggleFullscreen(m_window.get());

    glEnable(GL_DEPTH_TEST);

    // --- 立方体の頂点データと色データ (共有頂点に修正) ---
    float vertices[] = {
        // 位置 (XYZ)                                     色 (RGB)
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

    // --- Chunkの初期化 ---
    // m_gridSizeはApplication::initialize()で既に定義されているので、これを使用します。
    // densityはChunkコンストラクタで固定されるか、必要に応じて引数で渡します。
    try
    {
        // 例: サイズ16, 存在する確率0.3 (30%)
        m_chunk = std::make_unique<Chunk>(m_gridSize, 0.3f); // spacing引数を削除
    }
    catch (const std::exception &e)
    {
        std::cerr << "Failed to create Chunk: " << e.what() << std::endl;
        return false;
    }


    if (!m_fontLoader.loadSDFont("../assets/fonts/NotoSansJP-VariableFont_wght.json", "../assets/fonts/noto_sans_jp_atlas.png", m_fontData))
    {
        std::cerr << "Failed to load font data." << std::endl;
        return false;
    }

    // TextRendererの初期化
    if (!m_textRenderer.initialize("../shaders/text.vert", "../shaders/text.frag", m_fontData))
    {
        std::cerr << "Failed to initialize TextRenderer." << std::endl;
        return false;
    }

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
    // F11キーによるフルスクリーン切り替えはApplicationが担当
    static bool f11_pressed_last_frame = false;
    if (glfwGetKey(m_window.get(), GLFW_KEY_F11) == GLFW_PRESS)
    {
        if (!f11_pressed_last_frame)
        {
            m_fullscreenManager.toggleFullscreen(m_window.get());
            // フルスクリーン切り替え時のマウス状態リセットはFullscreenManagerのコールバックに任せる
        }
        f11_pressed_last_frame = true;
    }
    else
    {
        f11_pressed_last_frame = false;
    }

    // InputManagerの入力処理を呼び出す（Escキー処理が行われる）
    if (m_inputManager)
    {
        m_inputManager->processInput();
    }

    // WASDキーによる移動 (Cameraクラスに処理を委譲)
    // WASDのキー状態はApplicationで直接検出する
    bool forward = glfwGetKey(m_window.get(), GLFW_KEY_W) == GLFW_PRESS;
    bool backward = glfwGetKey(m_window.get(), GLFW_KEY_S) == GLFW_PRESS;
    bool left = glfwGetKey(m_window.get(), GLFW_KEY_A) == GLFW_PRESS;
    bool right = glfwGetKey(m_window.get(), GLFW_KEY_D) == GLFW_PRESS;

    m_camera.processMovementVector(forward, backward, left, right, m_timer.getDeltaTime());

    // スペースキーと左コントロールキーによる上下移動 (Cameraクラスに処理を委譲)
    bool moveUp = glfwGetKey(m_window.get(), GLFW_KEY_SPACE) == GLFW_PRESS;
    bool moveDown = glfwGetKey(m_window.get(), GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;

    m_camera.processVerticalMovement(moveUp, moveDown, m_timer.getDeltaTime());
}

// 更新処理
void Application::update()
{
    m_timer.tick();

    // FPS表示ロジック
    static double lastFPSTime = 0.0;
    static int frameCount = 0;

    frameCount++;
    if (m_timer.getDeltaTime() > 0.0)
    {
        if (m_timer.getTotalTime() - lastFPSTime >= 1.0)
        {
            double fps = (double)frameCount / (m_timer.getTotalTime() - lastFPSTime);
            m_fpsString = "FPS: " + std::to_string(static_cast<int>(fps));

            frameCount = 0;
            lastFPSTime = m_timer.getTotalTime();
        }
    }

    // カメラ座標表示ロジック
    glm::vec3 cameraPos = m_camera.getPosition();
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) // 小数点以下2桁に固定
       << "Pos: X: " << cameraPos.x          // ここにスペースを追加
       << " Y: " << cameraPos.y              // ここにスペースを追加
       << " Z: " << cameraPos.z;             // ここにスペースを追加
    m_positionString = ss.str();
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

    // 各ボクセルを描画
    // Chunkクラスからグリッドサイズとボクセルデータを取得
    if (m_chunk)
    { // m_chunkが有効であることを確認
        int chunkSize = m_chunk->getSize();
        // float cubeSpacing = m_chunk->getSpacing(); // ChunkからgetSpacing()を削除したので、m_cubeSpacingを直接使用

        for (int x = 0; x < chunkSize; ++x)
        {
            for (int y = 0; y < chunkSize; ++y)
            {
                for (int z = 0; z < chunkSize; ++z)
                {
                    if (m_chunk->getVoxel(x, y, z))
                    { // trueの場合のみ立方体を描画
                        glm::mat4 model = glm::mat4(1.0f);

                        // グリッド座標をワールド座標に変換し、中心を基準にオフセット
                        // m_cubeSpacingはApplicationのメンバー変数を使用
                        glm::vec3 cubeWorldPos = glm::vec3(x * m_cubeSpacing, y * m_cubeSpacing, z * m_cubeSpacing);

                        model = glm::translate(model, cubeWorldPos);

                        glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

                        // 立方体を描画
                        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
                    }
                }
            }
        }
    }

    glBindVertexArray(0); // 描画終了後にVAOをアンバインド

    // --- テキストレンダリング ---
    int width, height;
    glfwGetFramebufferSize(m_window.get(), &width, &height);
    glm::mat4 orthoProjection = glm::ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height));

    // テキストの目標表示高さ（ピクセル）
    const float TARGET_TEXT_HEIGHT_PX = 30.0f; // 例: 30ピクセル

    // フォントデータから読み込んだ基準フォントサイズに基づいてスケールを計算
    float textScale = 1.0f;
    if (m_fontData.baseFontSize > 0)
    {
        textScale = TARGET_TEXT_HEIGHT_PX / static_cast<float>(m_fontData.baseFontSize);
    }

    m_textRenderer.renderText(m_fpsString, 10.0f, static_cast<float>(height) - 60.0f, textScale, glm::vec3(1.0f, 1.0f, 1.0f), orthoProjection);

    // 座標表示 (FPSの下に配置)
    m_textRenderer.renderText(m_positionString,
                              10.0f,
                              (static_cast<float>(height) - 60.0f) - (TARGET_TEXT_HEIGHT_PX + 10.0f),
                              textScale,
                              glm::vec3(1.0f, 1.0f, 1.0f),
                              orthoProjection);
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
    if (app && app->m_inputManager) // InputManagerのインスタンスが有効か確認
    {
        app->m_inputManager->processMouseMovement(xposIn, yposIn);
    }
}
