#include "application.hpp"
#include <glad/glad.h>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread> // std::thread::hardware_concurrency() のために追加

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

Application::Application()
    : m_windowContext(std::make_unique<WindowContext>("Hello OpenGL Cubes", INITIAL_SCR_WIDTH, INITIAL_SCR_HEIGHT)),
      m_camera(std::make_unique<Camera>(glm::vec3(0.0f))),
      m_timer(std::make_unique<Timer>([]()
                                       { return glfwGetTime(); })),
      m_inputManager(std::make_unique<InputManager>(*m_camera)),
      m_fontLoader(std::make_unique<FontLoader>()),
      m_fontData(std::make_unique<FontData>()),
      // ここにワーカースレッドの数を追加
      m_chunkManager(std::make_unique<ChunkManager>(
          CHUNK_GRID_SIZE, RENDER_DISTANCE_CHUNKS, WORLD_SEED, NOISE_SCALE,
          WORLD_MAX_HEIGHT, GROUND_LEVEL, TERRAIN_OCTAVES, TERRAIN_LACUNARITY,
          TERRAIN_PERSISTENCE,
          std::thread::hardware_concurrency() > 0 ? std::thread::hardware_concurrency() : 4 // 10番目の引数
      )),
      m_renderer(std::make_unique<Renderer>(INITIAL_SCR_WIDTH, INITIAL_SCR_HEIGHT)),
      m_textRenderer(std::make_unique<TextRenderer>())
{
    // コールバックの設定 (ここで GLFW のウィンドウと InputManager を関連付ける)
    setupCallbacks();
}

Application::~Application()
{
    std::cout << "Application destructor called." << std::endl;
}

bool Application::initialize()
{
    // GLFW と GLAD の初期化
    if (!m_windowContext->initialize())
    {
        return false;
    }

    if (!m_windowContext->createWindow())
    {
        return false;
    }

    // OpenGL の関数ポインタをロード
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "GLAD initialization failed." << std::endl;
        return false;
    }
    std::cout << "GLAD initialized." << std::endl;

    m_inputManager->setWindow(m_windowContext->getWindow());
    m_inputManager->disableCursor();
    std::cout << "InputManager: Window set and cursor disabled." << std::endl;

    // OpenGL の設定
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE); // 背面カリングを有効にする
    glCullFace(GL_BACK);    // 背面をカリングする

    // プロジェクション行列の初期化
    m_projectionMatrix = glm::perspective(glm::radians(45.0f),
                                          (float)INITIAL_SCR_WIDTH / (float)INITIAL_SCR_HEIGHT, 0.1f, 1000.0f);

    // RendererとTextRendererの初期化はコンストラクタで行われるため、ここではリソースのロードなどを行う
    if (!setupDependenciesAndLoadResources())
    {
        std::cerr << "Failed to setup dependencies or load resources." << std::endl;
        return false;
    }

    // 初期チャンクの更新をトリガー（プレイヤーの初期位置）
    // update()が呼ばれる前にチャンクがロードされるように
    m_chunkManager->update(m_camera->getPosition());

    return true;
}

void Application::run()
{
    while (!m_windowContext->shouldClose())
    {
        processInput();
        update();
        render();
        m_windowContext->swapBuffers();
        m_windowContext->pollEvents();
    }
}

void Application::processInput()
{
    m_inputManager->processInput(m_timer->getDeltaTime());
}

void Application::update()
{
    m_timer->tick();
    updateFpsAndPositionStrings();

    // チャンクマネージャの更新（チャンクのロード/アンロード/メッシュ更新）
    m_chunkManager->update(m_camera->getPosition());
}

void Application::render()
{
    if (!m_renderer)
    {
        std::cerr << "Renderer is not initialized!\n";
        return;
    }

    m_renderer->beginFrame(
        glm::vec4(CLEAR_COLOR_R, CLEAR_COLOR_G, CLEAR_COLOR_B, CLEAR_COLOR_A));

    glm::mat4 view = m_camera->getViewMatrix();
    glm::mat4 viewProjection = m_projectionMatrix * view;

    extractFrustumPlanes(viewProjection);

    const auto &allRenderData = m_chunkManager->getAllRenderData();
    // デバッグ出力
    std::cout << "DEBUG: Number of chunks in render data map: " << allRenderData.size() << std::endl;

    int renderedChunkCount = 0;
    for (const auto &pair : allRenderData)
    {
        glm::ivec3 chunkCoord = pair.first;
        const ChunkRenderData &renderData = pair.second;

        if (!isChunkInFrustum(chunkCoord))
        {
            continue;
        }

        // デバッグ出力 (VAOが0でないことを確認)
        if (renderData.VAO == 0) {
            std::cerr << "ERROR: Chunk " << chunkCoord.x << "," << chunkCoord.y << "," << chunkCoord.z << " has VAO 0. Skipping render." << std::endl;
            continue;
        }

        glm::mat4 model = glm::translate(glm::mat4(1.0f),
                                         static_cast<glm::vec3>(chunkCoord * CHUNK_GRID_SIZE));

        m_renderer->renderScene(m_projectionMatrix, view, renderData, model);
        renderedChunkCount++;
    }
    // デバッグ出力
    std::cout << "DEBUG: Number of chunks actually rendered: " << renderedChunkCount << std::endl;

    int w, h;
    glfwGetFramebufferSize(m_windowContext->getWindow(), &w, &h);
    m_renderer->renderOverlay(w, h, *m_textRenderer, *m_fontData, m_fpsString, m_positionString);

    m_renderer->endFrame();
}

void Application::updateFpsAndPositionStrings()
{
    m_fpsString = "FPS: " + std::to_string(static_cast<int>(m_timer->getFPS()));

    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "POS: X: " << m_camera->getPosition().x
       << " Y: " << m_camera->getPosition().y
       << " Z: " << m_camera->getPosition().z;
    m_positionString = ss.str();
}

void Application::updateProjectionMatrix(int width, int height)
{
    m_projectionMatrix = glm::perspective(glm::radians(45.0f),
                                          (float)width / (float)height, 0.1f, 1000.0f);
    m_renderer->setViewport(0, 0, width, height); // ビューポートも更新
}

void Application::setupCallbacks()
{
    // Lambda 関数をキャプチャせずに静的関数としてコールバックを設定
    // GLFW からのフレームバッファサイズ変更コールバック
    glfwSetFramebufferSizeCallback(m_windowContext->getWindow(), [](GLFWwindow *window, int width, int height)
                                   {
        // ユーザーポインタから Application インスタンスを取得
        Application *app = static_cast<Application *>(glfwGetWindowUserPointer(window));
        if (app) {
            app->updateProjectionMatrix(width, height);
        } });

    // GLFW のウィンドウユーザーポインタとして Application インスタンスを保存
    glfwSetWindowUserPointer(m_windowContext->getWindow(), this);

    // 入力コールバックの設定
    m_inputManager->setupCallbacks(m_windowContext->getWindow());
}

bool Application::setupDependenciesAndLoadResources()
{
    // Renderer の初期化 (シェーダーのロードなど)
    if (!m_renderer->initialize())
    {
        std::cerr << "Failed to initialize Renderer." << std::endl;
        return false;
    }

    // TextRenderer の初期化 (フォントテクスチャのロードなど)
    if (!m_textRenderer->initialize(*m_fontLoader, *m_fontData))
    {
        std::cerr << "Failed to initialize TextRenderer." << std::endl;
        return false;
    }

    return true;
}

// 参照: https://www.lighthouse3d.com/tutorials/view-frustum-culling/clip-space-approach-extracting-planes/
void Application::extractFrustumPlanes(const glm::mat4 &viewProjection)
{
    // Right plane
    m_frustumPlanes[0].normal.x = viewProjection[0][3] - viewProjection[0][0];
    m_frustumPlanes[0].normal.y = viewProjection[1][3] - viewProjection[1][0];
    m_frustumPlanes[0].normal.z = viewProjection[2][3] - viewProjection[2][0];
    m_frustumPlanes[0].distance = viewProjection[3][3] - viewProjection[3][0];

    // Left plane
    m_frustumPlanes[1].normal.x = viewProjection[0][3] + viewProjection[0][0];
    m_frustumPlanes[1].normal.y = viewProjection[1][3] + viewProjection[1][0];
    m_frustumPlames[1].normal.z = viewProjection[2][3] + viewProjection[2][0];
    m_frustumPlanes[1].distance = viewProjection[3][3] + viewProjection[3][0];

    // Bottom plane
    m_frustumPlanes[2].normal.x = viewProjection[0][3] + viewProjection[0][1];
    m_frustumPlanes[2].normal.y = viewProjection[1][3] + viewProjection[1][1];
    m_frustumPlanes[2].normal.z = viewProjection[2][3] + viewProjection[2][1];
    m_frustumPlanes[2].distance = viewProjection[3][3] + viewProjection[3][1];

    // Top plane
    m_frustumPlanes[3].normal.x = viewProjection[0][3] - viewProjection[0][1];
    m_frustumPlanes[3].normal.y = viewProjection[1][3] - viewProjection[1][1];
    m_frustumPlames[3].normal.z = viewProjection[2][3] - viewProjection[2][1];
    m_frustumPlanes[3].distance = viewProjection[3][3] - viewProjection[3][1];

    // Far plane
    m_frustumPlanes[4].normal.x = viewProjection[0][3] - viewProjection[0][2];
    m_frustumPlanes[4].normal.y = viewProjection[1][3] - viewProjection[1][2];
    m_frustumPlanes[4].normal.z = viewProjection[2][3] - viewProjection[2][2];
    m_frustumPlanes[4].distance = viewProjection[3][3] - viewProjection[3][2];

    // Near plane
    m_frustumPlanes[5].normal.x = viewProjection[0][3] + viewProjection[0][2];
    m_frustumPlanes[5].normal.y = viewProjection[1][3] + viewProjection[1][2];
    m_frustumPlanes[5].normal.z = viewProjection[2][3] + viewProjection[2][2];
    m_frustumPlanes[5].distance = viewProjection[3][3] + viewProjection[3][2];

    // 各平面を正規化
    for (int i = 0; i < 6; ++i)
    {
        float length = glm::length(m_frustumPlanes[i].normal);
        m_frustumPlanes[i].normal /= length;
        m_frustumPlanes[i].distance /= length;
    }
}

bool Application::isChunkInFrustum(const glm::ivec3 &chunkCoord) const
{
    // AABB のワールド座標での最小点と最大点を計算
    glm::vec3 minP = static_cast<glm::vec3>(chunkCoord * CHUNK_GRID_SIZE);
    glm::vec3 maxP = minP + static_cast<glm::vec3>(CHUNK_GRID_SIZE);

    // 6つのカリング平面それぞれについてテスト
    for (int i = 0; i < 6; ++i)
    {
        const Plane &plane = m_frustumPlanes[i];

        // AABB の頂点のうち、平面に最も遠い点と最も近い点を計算
        glm::vec3 p_vertex = minP; // 最も近い点
        glm::vec3 n_vertex = maxP; // 最も遠い点

        if (plane.normal.x >= 0)
        {
            p_vertex.x = maxP.x;
            n_vertex.x = minP.x;
        }
        if (plane.normal.y >= 0)
        {
            p_vertex.y = maxP.y;
            n_vertex.y = minP.y;
        }
        if (plane.normal.z >= 0)
        {
            p_vertex.z = maxP.z;
            n_vertex.z = minP.z;
        }

        // 最も近い点が平面の外側にある場合、AABB は完全にカリングされる
        if (glm::dot(plane.normal, p_vertex) + plane.distance < 0)
        {
            return false;
        }
        // 最も遠い点が平面の内側にある場合、AABB は完全に平面の内側にある（必要であれば）
        // if (glm::dot(plane.normal, n_vertex) + plane.distance >= 0)
        // {
        //     // この平面ではカリングされない
        // }
    }
    return true; // どの平面でもカリングされなかった場合、フラスム内にある
}