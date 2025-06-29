#include "application.hpp"
#include <glad/glad.h>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

Application::Application()
    : m_windowContext(std::make_unique<WindowContext>("Hello OpenGL Cubes", INITIAL_SCR_WIDTH, INITIAL_SCR_HEIGHT)),
      m_camera(std::make_unique<Camera>(glm::vec3(0.0f))),
      m_timer(std::make_unique<Timer>()),
      m_inputManager(std::make_unique<InputManager>(*m_camera)),
      m_fontLoader(std::make_unique<FontLoader>()),
      m_fontData(std::make_unique<FontData>()),
      m_chunkManager(std::make_unique<ChunkManager>(
          CHUNK_GRID_SIZE, RENDER_DISTANCE_CHUNKS, WORLD_SEED, NOISE_SCALE,
          WORLD_MAX_HEIGHT, GROUND_LEVEL, TERRAIN_OCTAVES, TERRAIN_LACUNARITY,
          TERRAIN_PERSISTENCE)),
      m_renderer(std::make_unique<Renderer>()),
      m_projectionMatrix(1.0f)
{
}

Application::~Application()
{
    glfwTerminate();
}

bool Application::initialize()
{
    if (!m_windowContext->initialize())
    {
        std::cerr << "Application: Failed to initialize WindowContext. Exiting.\n";
        return false;
    }

    // ここにウィンドウを最大化するコードを追加します
    glfwMaximizeWindow(m_windowContext->getWindow());

    m_inputManager->setWindow(m_windowContext->getWindow());

    if (!m_fontLoader->loadSDFont("../assets/fonts/NotoSansJP-VariableFont_wght.json",
                                  "../assets/fonts/noto_sans_jp_atlas.png", *m_fontData))
    {
        std::cerr << "Application: Failed to load font data." << std::endl;
        return false;
    }

    if (!m_renderer->initialize(*m_fontData))
    {
        std::cerr << "Failed to initialize Renderer.\n";
        return false;
    }

    m_chunkManager->update(m_camera->getPosition());

    setupCallbacks();

    int initialWidth, initialHeight;
    glfwGetFramebufferSize(m_windowContext->getWindow(), &initialWidth, &initialHeight);
    updateProjectionMatrix(initialWidth, initialHeight);

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

void Application::setupCallbacks()
{
    m_windowContext->setFramebufferSizeCallback(
        [this](int w, int h)
        { updateProjectionMatrix(w, h); });
    m_windowContext->setCursorPosCallback([this](double xpos, double ypos)
                                          { m_inputManager->processMouseMovement(xpos, ypos); });
}

void Application::processInput()
{
    m_inputManager->processInput();

    auto key = [window = m_windowContext->getWindow()](int k)
    {
        return glfwGetKey(window, k) == GLFW_PRESS;
    };

    m_camera->processMovementVector(key(GLFW_KEY_W), key(GLFW_KEY_S), key(GLFW_KEY_A),
                                    key(GLFW_KEY_D), m_timer->getDeltaTime());
    m_camera->processVerticalMovement(key(GLFW_KEY_SPACE), key(GLFW_KEY_LEFT_CONTROL),
                                      m_timer->getDeltaTime());
}

void Application::update()
{
    m_timer->tick();
    updateFpsAndPositionStrings();
    m_chunkManager->update(m_camera->getPosition());
}

void Application::updateFpsAndPositionStrings()
{
    static double lastFPSTime = 0.0;
    static int frameCount = 0;
    frameCount++;

    if (m_timer->getTotalTime() - lastFPSTime >= 1.0)
    {
        double fps = frameCount / (m_timer->getTotalTime() - lastFPSTime);
        m_fpsString = "FPS: " + std::to_string(static_cast<int>(fps));
        frameCount = 0;
        lastFPSTime = m_timer->getTotalTime();
    }

    glm::vec3 pos = m_camera->getPosition();
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << "Pos: X: " << pos.x << " Y: " << pos.y
       << " Z: " << pos.z;
    m_positionString = ss.str();
}

void Application::extractFrustumPlanes(const glm::mat4 &viewProjection)
{
    m_frustumPlanes[0].normal = glm::vec3(viewProjection[0][3] - viewProjection[0][0],
                                          viewProjection[1][3] - viewProjection[1][0],
                                          viewProjection[2][3] - viewProjection[2][0]);
    m_frustumPlanes[0].distance = viewProjection[3][3] - viewProjection[3][0];

    m_frustumPlanes[1].normal = glm::vec3(viewProjection[0][3] + viewProjection[0][0],
                                          viewProjection[1][3] + viewProjection[1][0],
                                          viewProjection[2][3] + viewProjection[2][0]);
    m_frustumPlanes[1].distance = viewProjection[3][3] + viewProjection[3][0];

    m_frustumPlanes[2].normal = glm::vec3(viewProjection[0][3] + viewProjection[0][1],
                                          viewProjection[1][3] + viewProjection[1][1],
                                          viewProjection[2][3] + viewProjection[2][1]);
    m_frustumPlanes[2].distance = viewProjection[3][3] + viewProjection[3][1];

    m_frustumPlanes[3].normal = glm::vec3(viewProjection[0][3] - viewProjection[0][1],
                                          viewProjection[1][3] - viewProjection[1][1],
                                          viewProjection[2][3] - viewProjection[2][1]);
    m_frustumPlanes[3].distance = viewProjection[3][3] - viewProjection[3][1];

    m_frustumPlanes[4].normal = glm::vec3(viewProjection[0][3] - viewProjection[0][2],
                                          viewProjection[1][3] - viewProjection[1][2],
                                          viewProjection[2][3] - viewProjection[2][2]);
    m_frustumPlanes[4].distance = viewProjection[3][3] - viewProjection[3][2];

    m_frustumPlanes[5].normal = glm::vec3(viewProjection[0][3] + viewProjection[0][2],
                                          viewProjection[1][3] + viewProjection[1][2],
                                          viewProjection[2][3] + viewProjection[2][2]);
    m_frustumPlanes[5].distance = viewProjection[3][3] + viewProjection[3][2];

    for (int i = 0; i < 6; ++i)
    {
        float length = glm::length(m_frustumPlanes[i].normal);
        m_frustumPlanes[i].normal /= length;
        m_frustumPlanes[i].distance /= length;
    }
}

bool Application::isChunkInFrustum(const glm::ivec3 &chunkCoord) const
{
    glm::vec3 minPoint = static_cast<glm::vec3>(chunkCoord * CHUNK_GRID_SIZE);
    glm::vec3 maxPoint = static_cast<glm::vec3>((chunkCoord + glm::ivec3(1)) * CHUNK_GRID_SIZE);

    for (int i = 0; i < 6; ++i)
    {
        const Plane &p = m_frustumPlanes[i];

        glm::vec3 p_vertex = minPoint;
        glm::vec3 n_vertex = maxPoint;

        if (p.normal.x >= 0)
        {
            p_vertex.x = maxPoint.x;
            n_vertex.x = minPoint.x;
        }
        if (p.normal.y >= 0)
        {
            p_vertex.y = maxPoint.y;
            n_vertex.y = minPoint.y;
        }
        if (p.normal.z >= 0)
        {
            p_vertex.z = maxPoint.z;
            n_vertex.z = minPoint.z;
        }

        if (glm::dot(p.normal, p_vertex) + p.distance < 0)
        {
            return false;
        }
    }
    return true;
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
    for (const auto &pair : allRenderData)
    {
        glm::ivec3 chunkCoord = pair.first;
        const ChunkRenderData &renderData = pair.second;

        if (!isChunkInFrustum(chunkCoord))
        {
            continue;
        }

        glm::mat4 model = glm::translate(glm::mat4(1.0f),
                                         static_cast<glm::vec3>(chunkCoord * CHUNK_GRID_SIZE));

        m_renderer->renderScene(m_projectionMatrix, view, renderData, model);
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
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    m_projectionMatrix = glm::perspective(glm::radians(m_camera->Zoom), aspect, 0.1f, 1000.0f);
}