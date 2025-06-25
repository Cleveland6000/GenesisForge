// src/window_context.cpp

#include "window_context.hpp" // これを最初にインクルード

// コンストラクタ
WindowContext::WindowContext(const std::string& title, int width, int height)
    : m_window(nullptr, glfwDestroyWindow), // カスタムデリータを設定
      m_title(title),
      m_width(width),
      m_height(height)
{
    // コンストラクタではGLFWの初期化は行わない。initialize() で行う。
    std::cout << "--- WindowContext constructor called. ---\n";
}

// デストラクタ
WindowContext::~WindowContext() {
    // m_window のデリータが glfwDestroyWindow を呼び出す
    // glfwTerminate() は WindowContext::initialize() の中でglfwInit()が成功した場合に、
    // GameApplicationのデストラクタか、main関数の終了時に一度だけ呼ばれるべき。
    // ここでは呼ばない。
    std::cout << "--- WindowContext destructor called. ---\n";
}

// 初期化メソッド
bool WindowContext::initialize() {
    // GLFWの初期化
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }
    std::cout << "GLFW initialized.\n";

    // ウィンドウヒントの設定
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4); // MSAA (マルチサンプルアンチエイリアシング)

    // ウィンドウの作成とOpenGLコンテキストの生成
    m_window.reset(glfwCreateWindow(m_width, m_height, m_title.c_str(), NULL, NULL));
    if (!m_window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate(); // ウィンドウ作成失敗時はGLFWを終了
        return false;
    }
    std::cout << "GLFW window created.\n";

    // ウィンドウをスクリーン中央に配置 (オプション)
    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primary);
    glfwSetWindowPos(m_window.get(), (mode->width - m_width) / 2, (mode->height - m_height) / 2);

    // 作成したウィンドウを現在のコンテキストにする
    glfwMakeContextCurrent(m_window.get());
    glfwSwapInterval(0); // V-Sync 無効 (必要に応じて1に設定)

    // GLADの初期化
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        // ここではglfwTerminate()を呼ばない。WindowContextが責任を持つのはウィンドウ作成まで。
        // GLFW全体の終了はGameApplicationかmainで責任を持つ。
        return false;
    }
    std::cout << "GLAD initialized.\n";

    // ウィンドウユーザーポインタを設定し、コールバック関数を登録
    glfwSetWindowUserPointer(m_window.get(), this);
    glfwSetFramebufferSizeCallback(m_window.get(), WindowContext::staticFramebufferSizeCallback);
    glfwSetCursorPosCallback(m_window.get(), WindowContext::staticCursorPosCallback);

    return true;
}

// 静的フレームバッファサイズコールバック
void WindowContext::staticFramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    auto* self = static_cast<WindowContext*>(glfwGetWindowUserPointer(window));
    if (self && self->m_framebufferSizeCallback) {
        self->m_framebufferSizeCallback(width, height);
    }
    // OpenGLのビューポートもここで更新する
    glViewport(0, 0, width, height);
}

// 静的マウスカーソル位置コールバック
void WindowContext::staticCursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    auto* self = static_cast<WindowContext*>(glfwGetWindowUserPointer(window));
    if (self && self->m_cursorPosCallback) {
        self->m_cursorPosCallback(xpos, ypos);
    }
}