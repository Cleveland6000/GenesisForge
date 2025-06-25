#include <glad/glad.h>
#include "application.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>

const float Application::CLEAR_COLOR_R = 0.0f, Application::CLEAR_COLOR_G = 0.0f, Application::CLEAR_COLOR_B = 0.0f, Application::CLEAR_COLOR_A = 1.0f;

Application::Application()
    : m_window(nullptr, glfwDestroyWindow),
      m_camera(glm::vec3(0.0f)),
      m_timer(), m_fontLoader(), m_inputManager(nullptr),
      m_chunk(nullptr), m_renderer(nullptr) {}

Application::~Application() { glfwTerminate(); }

bool Application::initialize()
{
    return initGLFW() && createWindowAndContext() && (setupCallbacks(), true) && initializeManagers() && initializeChunkAndFont() && initializeRenderer();
}

bool Application::initGLFW()
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    return true;
}

bool Application::createWindowAndContext()
{
    m_window.reset(glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Hello OpenGL Cubes", NULL, NULL));
    if (!m_window)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return false;
    }
    GLFWmonitor *primary = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(primary);
    glfwSetWindowPos(m_window.get(), (mode->width - SCR_WIDTH) / 2, (mode->height - SCR_HEIGHT) / 2);
    glfwMakeContextCurrent(m_window.get());
    glfwSwapInterval(0);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD\n";
        return false;
    }
    return true;
}

void Application::setupCallbacks()
{
    auto win = m_window.get();
    glfwSetWindowUserPointer(win, this);
    glfwSetFramebufferSizeCallback(win, Application::staticFramebufferSizeCallback);
    glfwSetCursorPosCallback(win, Application::staticMouseCallback);
}

bool Application::initializeManagers()
{
    m_inputManager = std::make_unique<InputManager>(m_window.get(), m_camera);
    m_fullscreenManager.setWindowSizeChangeCallback([](int w, int h)
                                                    {
        if (auto win = glfwGetCurrentContext()) {
            if (auto app = static_cast<Application *>(glfwGetWindowUserPointer(win)))
                app->updateProjectionMatrix(w, h);
        } });
    m_fullscreenManager.setMouseResetCallback([]()
                                              {
        if (auto win = glfwGetCurrentContext()) {
            if (auto app = static_cast<Application *>(glfwGetWindowUserPointer(win)); app && app->m_inputManager)
                app->m_inputManager->resetMouseState();
        } });
    m_fullscreenManager.toggleFullscreen(m_window.get());
    return true;
}

bool Application::initializeChunkAndFont()
{
    try
    {
        m_chunk = std::make_unique<Chunk>(m_gridSize, 0.3f);
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
    return true;
}

bool Application::initializeRenderer()
{
    m_renderer = std::make_unique<Renderer>();
    if (!m_renderer->initialize(m_fontData))
    {
        std::cerr << "Failed to initialize Renderer.\n";
        return false;
    }
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
    static bool f11_last = false;
    if (glfwGetKey(m_window.get(), GLFW_KEY_F11) == GLFW_PRESS)
    {
        if (!f11_last)
            m_fullscreenManager.toggleFullscreen(m_window.get());
        f11_last = true;
    }
    else
        f11_last = false;

    if (m_inputManager)
        m_inputManager->processInput();

    auto key = [this](int k)
    { return glfwGetKey(m_window.get(), k) == GLFW_PRESS; };
    m_camera.processMovementVector(key(GLFW_KEY_W), key(GLFW_KEY_S), key(GLFW_KEY_A), key(GLFW_KEY_D), m_timer.getDeltaTime());
    m_camera.processVerticalMovement(key(GLFW_KEY_SPACE), key(GLFW_KEY_LEFT_CONTROL), m_timer.getDeltaTime());
}

void Application::update()
{
    m_timer.tick();
    updateFpsAndPositionStrings();
}

void Application::updateFpsAndPositionStrings()
{
    static double lastFPSTime = 0.0;
    static int frameCount = 0;
    frameCount++;
    if (m_timer.getDeltaTime() > 0.0 && m_timer.getTotalTime() - lastFPSTime >= 1.0)
    {
        double fps = frameCount / (m_timer.getTotalTime() - lastFPSTime);
        m_fpsString = "FPS: " + std::to_string(static_cast<int>(fps));
        frameCount = 0;
        lastFPSTime = m_timer.getTotalTime();
    }
    glm::vec3 pos = m_camera.getPosition();
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << "Pos: X: " << pos.x << " Y: " << pos.y << " Z: " << pos.z;
    m_positionString = ss.str();
}

void Application::render()
{
    if (!m_renderer)
    {
        std::cerr << "Renderer is not initialized!\n";
        return;
    }
    m_renderer->beginFrame(glm::vec4(CLEAR_COLOR_R, CLEAR_COLOR_G, CLEAR_COLOR_B, CLEAR_COLOR_A));
    glm::mat4 view = m_camera.getViewMatrix();
    m_renderer->renderScene(m_projectionMatrix, view, *m_chunk, m_cubeSpacing);
    int w, h;
    glfwGetFramebufferSize(m_window.get(), &w, &h);
    m_renderer->renderOverlay(w, h, m_fpsString, m_positionString);
    m_renderer->endFrame();
}

void Application::staticFramebufferSizeCallback(GLFWwindow *window, int width, int height)
{
    if (auto app = static_cast<Application *>(glfwGetWindowUserPointer(window)))
        app->updateProjectionMatrix(width, height);
    glViewport(0, 0, width, height);
}

void Application::updateProjectionMatrix(int width, int height)
{
    if (width == 0 || height == 0)
        return;
    float aspect = (float)width / (float)height;
    m_projectionMatrix = glm::perspective(glm::radians(m_camera.Zoom), aspect, 0.1f, 100.0f);
}

void Application::staticMouseCallback(GLFWwindow *window, double xpos, double ypos)
{
    if (auto app = static_cast<Application *>(glfwGetWindowUserPointer(window)); app && app->m_inputManager)
        app->m_inputManager->processMouseMovement(xpos, ypos);
}
