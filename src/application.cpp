#include <glad/glad.h>
#include "application.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>

// GLMの実験的拡張機能を使用するための定義
// GLMのヘッダーをインクルードする前に定義する必要があります
#define GLM_ENABLE_EXPERIMENTAL 
#include <glm/gtx/norm.hpp> // glm::length のために必要 (glm/glm.hpp で含まれることが多いですが、明示的に)


// SCR_WIDTH と SCR_HEIGHT は通常、Application::initialize で WindowContext に渡されます。
// これらのマクロがどこかで定義されているか、あるいは直接定数として使用されているかを
// 確認してください。ここでは仮に存在するものとします。
// #define SCR_WIDTH 800
// #define SCR_HEIGHT 600

Application::Application()
    // m_projectionMatrix をコンストラクタで初期化する
    // C26495 警告の解消
    : m_projectionMatrix(1.0f) 
{
    // std::cout はデバッグ用途です。リリースビルドではコメントアウトまたは無効化してください。
    std::cout << "--- Application constructor called. ---\n";
}

Application::~Application()
{
    std::cout << "--- Application destructor called. ---\n";
    glfwTerminate();
}

bool Application::initialize()
{
    if (!setupDependenciesAndLoadResources())
    {
        std::cerr << "Application: Failed to setup dependencies and load resources. Exiting.\n";
        return false;
    }
    setupCallbacks();

    if (!m_fontLoader || !m_fontData)
    {
        std::cerr << "Error: FontLoader or FontData is null in Application::initialize.\n";
        return false;
    }

    // フォントロードのパスを確認してください。
    // 前回「Failed to open font json file」エラーが出た点です。
    if (!m_fontLoader->loadSDFont("../assets/fonts/NotoSansJP-VariableFont_wght.json", "../assets/fonts/noto_sans_jp_atlas.png", *m_fontData))
    {
        std::cerr << "Application: Failed to load font data." << std::endl;
        return false;
    }
    std::cout << "FontData loaded by Application.\n";

    if (m_renderer && !m_renderer->initialize(*m_fontData))
    {
        std::cerr << "Failed to initialize Renderer.\n";
        return false;
    }

    // 初期チャンクのロードをトリガー
    if (m_chunkManager) {
        m_chunkManager->update(m_camera->getPosition());
    } else {
        std::cerr << "Error: ChunkManager is null in Application::initialize after setupDependenciesAndLoadResources.\n";
        return false;
    }

    int initialWidth, initialHeight;
    glfwGetFramebufferSize(m_windowContext->getWindow(), &initialWidth, &initialHeight);
    updateProjectionMatrix(initialWidth, initialHeight);

    std::cout << "Application initialized successfully.\n";
    return true;
}

void Application::run()
{
    while (!m_windowContext->shouldClose())
    {
        processInput();
        update();
        render(); // ここで描画が行われる
        m_windowContext->swapBuffers();
        m_windowContext->pollEvents();
    }
}

void Application::setupCallbacks()
{
    m_windowContext->setFramebufferSizeCallback([this](int w, int h)
                                                 { this->updateProjectionMatrix(w, h); });
    m_windowContext->setCursorPosCallback([this](double xpos, double ypos)
                                           {
        if (this->m_inputManager) {
            this->m_inputManager->processMouseMovement(xpos, ypos);
        } });
}

bool Application::setupDependenciesAndLoadResources()
{
    std::cout << "Application::setupDependenciesAndLoadResources started.\n";

    m_windowContext = std::make_unique<WindowContext>("Hello OpenGL Cubes", SCR_WIDTH, SCR_HEIGHT);
    if (!m_windowContext || !m_windowContext->initialize())
    {
        std::cerr << "Application: Failed to create or initialize WindowContext.\n";
        return false;
    }
    std::cout << "WindowContext created and initialized.\n";

    m_camera = std::make_unique<Camera>(glm::vec3(0.0f));
    if (!m_camera)
    {
        std::cerr << "Application: Failed to create Camera.\n";
        return false;
    }
    std::cout << "Camera created.\n";

    m_timer = std::make_unique<Timer>();
    if (!m_timer)
    {
        std::cerr << "Application: Failed to create Timer.\n";
        return false;
    }
    std::cout << "Timer created.\n";

    m_inputManager = std::make_unique<InputManager>(*m_camera);
    if (!m_inputManager)
    {
        std::cerr << "Application: Failed to create InputManager.\n";
        return false;
    }
    std::cout << "InputManager created.\n";

    m_fullscreenManager = std::make_unique<FullscreenManager>();
    if (!m_fullscreenManager)
    {
        std::cerr << "Application: Failed to create FullscreenManager.\n";
        return false;
    }
    std::cout << "FullscreenManager created.\n";

    m_fontLoader = std::make_unique<FontLoader>();
    if (!m_fontLoader)
    {
        std::cerr << "Application: Failed to create FontLoader.\n";
        return false;
    }
    std::cout << "FontLoader created.\n";

    m_fontData = std::make_unique<FontData>();
    if (!m_fontData)
    {
        std::cerr << "Application: Failed to create FontData unique_ptr.\n";
        return false;
    }
    std::cout << "FontData unique_ptr created.\n";

    // ChunkManager の初期化時に、X/ZとY軸両方の描画距離を渡す
    m_chunkManager = std::make_unique<ChunkManager>(
        CHUNK_GRID_SIZE, 
        RENDER_DISTANCE_CHUNKS,       // X/Z軸方向の描画距離
        WORLD_SEED, 
        NOISE_SCALE, 
        WORLD_MAX_HEIGHT, 
        GROUND_LEVEL,
        TERRAIN_OCTAVES,      
        TERRAIN_LACUNARITY,     
        TERRAIN_PERSISTENCE     
    );
    if (!m_chunkManager)
    {
        std::cerr << "Application: Failed to create ChunkManager.\n";
        return false;
    }
    std::cout << "ChunkManager created with terrain generation parameters and octaves.\n";

    m_renderer = std::make_unique<Renderer>();
    if (!m_renderer)
    {
        std::cerr << "Application: Failed to create Renderer.\n";
        return false;
    }
    std::cout << "Renderer created.\n";

    m_inputManager->setWindow(m_windowContext->getWindow());
    m_fullscreenManager->setWindowSizeChangeCallback([this](int w, int h)
                                                     { this->updateProjectionMatrix(w, h); });
    m_fullscreenManager->setMouseResetCallback([this]()
                                                 {
        if (this->m_inputManager) {
            this->m_inputManager->resetMouseState();
        } });
    m_fullscreenManager->toggleFullscreen(m_windowContext->getWindow());

    std::cout << "Application::setupDependenciesAndLoadResources finished successfully.\n";
    return true;
}

void Application::processInput()
{
    static bool f11_last = false;
    if (glfwGetKey(m_windowContext->getWindow(), GLFW_KEY_F11) == GLFW_PRESS)
    {
        if (!f11_last)
            m_fullscreenManager->toggleFullscreen(m_windowContext->getWindow());
        f11_last = true;
    }
    else
        f11_last = false;

    if (m_inputManager)
        m_inputManager->processInput();

    auto key = [this](int k)
    { return glfwGetKey(m_windowContext->getWindow(), k) == GLFW_PRESS; };

    m_camera->processMovementVector(key(GLFW_KEY_W), key(GLFW_KEY_S), key(GLFW_KEY_A), key(GLFW_KEY_D), m_timer->getDeltaTime());
    m_camera->processVerticalMovement(key(GLFW_KEY_SPACE), key(GLFW_KEY_LEFT_CONTROL), m_timer->getDeltaTime());
}

void Application::update()
{
    m_timer->tick();
    updateFpsAndPositionStrings();

    // プレイヤーの位置に基づいてチャンクマネージャーを更新
    if (m_chunkManager) {
        m_chunkManager->update(m_camera->getPosition());
    }
}

void Application::updateFpsAndPositionStrings()
{
    static double lastFPSTime = 0.0;
    static int frameCount = 0;
    frameCount++;

    if (m_timer->getDeltaTime() > 0.0 && m_timer->getTotalTime() - lastFPSTime >= 1.0)
    {
        double fps = frameCount / (m_timer->getTotalTime() - lastFPSTime);
        m_fpsString = "FPS: " + std::to_string(static_cast<int>(fps));
        frameCount = 0;
        lastFPSTime = m_timer->getTotalTime();
    }

    glm::vec3 pos = m_camera->getPosition();
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << "Pos: X: " << pos.x << " Y: " << pos.y << " Z: " << pos.z;
    m_positionString = ss.str();
}

// 視錐台の平面を結合行列から抽出する関数
void Application::extractFrustumPlanes(const glm::mat4& viewProjection) {
    // 結合行列から各平面の情報を抽出
    // 平面の方程式 Ax + By + Cz + D = 0 に対応
    // normal = (A, B, C), distance = D

    // 右平面 (Right Plane): viewProjection[0][3] - viewProjection[0][0]
    m_frustumPlanes[0].normal.x = viewProjection[0][3] - viewProjection[0][0];
    m_frustumPlanes[0].normal.y = viewProjection[1][3] - viewProjection[1][0];
    m_frustumPlanes[0].normal.z = viewProjection[2][3] - viewProjection[2][0];
    m_frustumPlanes[0].distance = viewProjection[3][3] - viewProjection[3][0];

    // 左平面 (Left Plane): viewProjection[0][3] + viewProjection[0][0]
    m_frustumPlanes[1].normal.x = viewProjection[0][3] + viewProjection[0][0];
    m_frustumPlanes[1].normal.y = viewProjection[1][3] + viewProjection[1][0];
    m_frustumPlanes[1].normal.z = viewProjection[2][3] + viewProjection[2][0];
    m_frustumPlanes[1].distance = viewProjection[3][3] + viewProjection[3][0];

    // 下平面 (Bottom Plane): viewProjection[0][3] + viewProjection[0][1]
    m_frustumPlanes[2].normal.x = viewProjection[0][3] + viewProjection[0][1];
    m_frustumPlanes[2].normal.y = viewProjection[1][3] + viewProjection[1][1];
    m_frustumPlanes[2].normal.z = viewProjection[2][3] + viewProjection[2][1];
    m_frustumPlanes[2].distance = viewProjection[3][3] + viewProjection[3][1];

    // 上平面 (Top Plane): viewProjection[0][3] - viewProjection[0][1]
    m_frustumPlanes[3].normal.x = viewProjection[0][3] - viewProjection[0][1];
    m_frustumPlanes[3].normal.y = viewProjection[1][3] - viewProjection[1][1];
    m_frustumPlanes[3].normal.z = viewProjection[2][3] - viewProjection[2][1];
    m_frustumPlanes[3].distance = viewProjection[3][3] - viewProjection[3][1];

    // 遠平面 (Far Plane): viewProjection[0][3] - viewProjection[0][2]
    m_frustumPlanes[4].normal.x = viewProjection[0][3] - viewProjection[0][2];
    m_frustumPlanes[4].normal.y = viewProjection[1][3] - viewProjection[1][2];
    m_frustumPlanes[4].normal.z = viewProjection[2][3] - viewProjection[2][2];
    m_frustumPlanes[4].distance = viewProjection[3][3] - viewProjection[3][2];

    // 近平面 (Near Plane): viewProjection[0][3] + viewProjection[0][2]
    m_frustumPlanes[5].normal.x = viewProjection[0][3] + viewProjection[0][2];
    m_frustumPlanes[5].normal.y = viewProjection[1][3] + viewProjection[1][2];
    m_frustumPlanes[5].normal.z = viewProjection[2][3] + viewProjection[2][2];
    m_frustumPlanes[5].distance = viewProjection[3][3] + viewProjection[3][2];

    // すべての平面を正規化する（重要）
    for (int i = 0; i < 6; ++i) {
        float length = glm::length(m_frustumPlanes[i].normal);
        m_frustumPlanes[i].normal /= length;
        m_frustumPlanes[i].distance /= length;
    }
}

// チャンクが視錐台内にあるかテストする関数
// chunkCoord はチャンクのグリッド座標 (glm::ivec3)
bool Application::isChunkInFrustum(const glm::ivec3& chunkCoord) const {
    // チャンクのワールド座標におけるAABBの最小点と最大点を計算
    // CHUNK_GRID_SIZE はチャンクの1辺のボクセル数
    glm::vec3 minPoint = glm::vec3(
        static_cast<float>(chunkCoord.x * CHUNK_GRID_SIZE),
        static_cast<float>(chunkCoord.y * CHUNK_GRID_SIZE),
        static_cast<float>(chunkCoord.z * CHUNK_GRID_SIZE)
    );
    glm::vec3 maxPoint = glm::vec3(
        static_cast<float>((chunkCoord.x + 1) * CHUNK_GRID_SIZE),
        static_cast<float>((chunkCoord.y + 1) * CHUNK_GRID_SIZE),
        static_cast<float>((chunkCoord.z + 1) * CHUNK_GRID_SIZE)
    );

    // 各平面に対してテスト
    for (int i = 0; i < 6; ++i) {
        const Plane& p = m_frustumPlanes[i];

        // AABB と平面の交差テスト (P-vertex / N-vertex method)
        // AABBの8つの頂点のうち、1つでも平面の内側にあるかチェック
        // 最適化として、平面法線とのドット積が最も遠い頂点と最も近い頂点を考慮する

        glm::vec3 p_vertex = minPoint; // 平面法線と同じ方向のAABBの頂点
        glm::vec3 n_vertex = maxPoint; // 平面法線と逆方向のAABBの頂点

        if (p.normal.x >= 0) {
            p_vertex.x = maxPoint.x;
            n_vertex.x = minPoint.x;
        }
        if (p.normal.y >= 0) {
            p_vertex.y = maxPoint.y;
            n_vertex.y = minPoint.y;
        }
        if (p.normal.z >= 0) {
            p_vertex.z = maxPoint.z;
            n_vertex.z = minPoint.z;
        }

        // AABBの最も遠い頂点 (p_vertex) が平面の裏側にある場合、AABBは完全に平面の裏側にある
        // => 視錐台の外側にあると判断
        if (glm::dot(p.normal, p_vertex) + p.distance < 0) {
            return false; // この平面の外側にあるので、視錐台の外
        }
    }
    return true; // すべての平面の内側にある可能性がある
}

void Application::render()
{
    if (!m_renderer)
    {
        std::cerr << "Renderer is not initialized!\n";
        return;
    }

    m_renderer->beginFrame(glm::vec4(CLEAR_COLOR_R, CLEAR_COLOR_G, CLEAR_COLOR_B, CLEAR_COLOR_A));

    glm::mat4 view = m_camera->getViewMatrix();
    // ここで結合行列 (View-Projection Matrix) を計算
    glm::mat4 viewProjection = m_projectionMatrix * view;

    // 毎フレーム、視錐台の平面を抽出する
    // これを呼ぶことで m_frustumPlanes が更新される
    extractFrustumPlanes(viewProjection); 

    // ChunkManager からすべてのチャンクのレンダリングデータを取得し、ループして描画
    if (m_chunkManager) {
        const auto& allRenderData = m_chunkManager->getAllRenderData();
        for (const auto& pair : allRenderData) {
            glm::ivec3 chunkCoord = pair.first;
            const ChunkRenderData& renderData = pair.second; // renderData を取得

            // !!!! フラスタムカリングのチェックを追加 !!!!
            if (!isChunkInFrustum(chunkCoord)) {
                // チャンクが視錐台の外にある場合、このチャンクの描画をスキップ
                continue; 
            }

            // チャンクのワールド位置を計算し、model 行列を作成
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(
                chunkCoord.x * CHUNK_GRID_SIZE,
                chunkCoord.y * CHUNK_GRID_SIZE, 
                chunkCoord.z * CHUNK_GRID_SIZE
            ));
            
            // Renderer::renderScene に model 行列とレンダリングデータを渡して呼び出す
            // renderScene 内部で indexCount > 0 のチェックが必要です (既に実装済み)
            m_renderer->renderScene(m_projectionMatrix, view, renderData, model);
        }
    }


    int w, h;
    glfwGetFramebufferSize(m_windowContext->getWindow(), &w, &h);
    m_renderer->renderOverlay(w, h, m_fpsString, m_positionString);

    m_renderer->endFrame();
}

void Application::updateProjectionMatrix(int width, int height)
{
    if (width == 0 || height == 0)
        return;
    float aspect = (float)width / (float)height;
    m_projectionMatrix = glm::perspective(glm::radians(m_camera->Zoom), aspect, 0.1f, 1000.0f); // 遠景クリップ面を大きくする
}