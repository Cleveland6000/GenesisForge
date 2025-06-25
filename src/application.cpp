#include <glad/glad.h>
#include "application.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>

const float Application::CLEAR_COLOR_R = 0.0f;
const float Application::CLEAR_COLOR_G = 0.0f;
const float Application::CLEAR_COLOR_B = 0.0f;
const float Application::CLEAR_COLOR_A = 1.0f;

Application::Application()
    : m_window(nullptr, glfwDestroyWindow),
      m_camera(glm::vec3(0.0f, 0.0f, 0.0f)),
      m_timer(),
      m_fontLoader(),
      m_inputManager(nullptr),
      m_chunk(nullptr),
      m_renderer(nullptr)
{
}

Application::~Application()
{
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

    m_window.reset(glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Hello OpenGL Cubes", NULL, NULL));
    if (!m_window)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return false;
    }
    GLFWmonitor *primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(primaryMonitor);

    int windowPosX = (mode->width - SCR_WIDTH) / 2;
    int windowPosY = (mode->height - SCR_HEIGHT) / 2;

    glfwSetWindowPos(m_window.get(), windowPosX, windowPosY);

    glfwMakeContextCurrent(m_window.get());
    glfwSwapInterval(0);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD\n";
        return false;
    }

    glfwSetWindowUserPointer(m_window.get(), this);
    glfwSetFramebufferSizeCallback(m_window.get(), Application::staticFramebufferSizeCallback);
    glfwSetCursorPosCallback(m_window.get(), Application::staticMouseCallback);

    m_inputManager = std::make_unique<InputManager>(m_window.get(), m_camera);

    m_fullscreenManager.setWindowSizeChangeCallback([](int width, int int_height)
                                                    {
        GLFWwindow *currentWindow = glfwGetCurrentContext();
        if (currentWindow)
        {
            Application *app = static_cast<Application *>(glfwGetWindowUserPointer(currentWindow));
            if (app)
            {
                app->updateProjectionMatrix(width, int_height);
            }
        } });

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
    static bool f11_pressed_last_frame = false;
    if (glfwGetKey(m_window.get(), GLFW_KEY_F11) == GLFW_PRESS)
    {
        if (!f11_pressed_last_frame)
        {
            m_fullscreenManager.toggleFullscreen(m_window.get());
        }
        f11_pressed_last_frame = true;
    }
    else
    {
        f11_pressed_last_frame = false;
    }

    if (m_inputManager)
    {
        m_inputManager->processInput();
    }

    bool forward = glfwGetKey(m_window.get(), GLFW_KEY_W) == GLFW_PRESS;
    bool backward = glfwGetKey(m_window.get(), GLFW_KEY_S) == GLFW_PRESS;
    bool left = glfwGetKey(m_window.get(), GLFW_KEY_A) == GLFW_PRESS;
    bool right = glfwGetKey(m_window.get(), GLFW_KEY_D) == GLFW_PRESS;

    m_camera.processMovementVector(forward, backward, left, right, m_timer.getDeltaTime());

    bool moveUp = glfwGetKey(m_window.get(), GLFW_KEY_SPACE) == GLFW_PRESS;
    bool moveDown = glfwGetKey(m_window.get(), GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;

    m_camera.processVerticalMovement(moveUp, moveDown, m_timer.getDeltaTime());
}

void Application::update()
{
    m_timer.tick();

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

    glm::vec3 cameraPos = m_camera.getPosition();
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2)
       << "Pos: X: " << cameraPos.x
       << " Y: " << cameraPos.y
       << " Z: " << cameraPos.z;
    m_positionString = ss.str();
}

// 描画処理
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

    // 2Dオーバーレイ（テキスト）のレンダリング
    int width, height;
    glfwGetFramebufferSize(m_window.get(), &width, &height);
    m_renderer->renderOverlay(width, height, m_fpsString, m_positionString);

    // レンダリング終了（この関数では何もしないが、将来的に何か追加する場合に備えて）
    m_renderer->endFrame();
}

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

void Application::staticMouseCallback(GLFWwindow *window, double xposIn, double yposIn)
{
    Application *app = static_cast<Application *>(glfwGetWindowUserPointer(window));
    if (app && app->m_inputManager) // InputManagerのインスタンスが有効か確認
    {
        app->m_inputManager->processMouseMovement(xposIn, yposIn);
    }
}