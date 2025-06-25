// src/window_context.hpp

#ifndef WINDOW_CONTEXT_HPP
#define WINDOW_CONTEXT_HPP

#define GLFW_INCLUDE_NONE // これをGLFWの前に置く！
#include <glad/glad.h>   // これを一番最初に置く

#include <GLFW/glfw3.h> // GLFWのインクルード
#include <string>
#include <memory> // std::unique_ptr のために必要
#include <functional> // std::function のために必要
#include <iostream> // ログ出力のために必要

// 画面の幅と高さ (必要であればどこかで定義)
// 例えば、application.hpp から移動するか、共通のヘッダーファイルに置く
// 現時点では便宜上ここに仮置きします
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;


class WindowContext {
public:
    // ウィンドウポインタを解放するためのカスタムデリータ
    using GlfwWindowPtr = std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)>;

    WindowContext(const std::string& title, int width, int height);
    ~WindowContext();

    bool initialize();
    GLFWwindow* getWindow() const { return m_window.get(); }
    bool shouldClose() const { return glfwWindowShouldClose(m_window.get()); }
    void swapBuffers() { glfwSwapBuffers(m_window.get()); }
    void pollEvents() { glfwPollEvents(); }

    // コールバックのためのセッター
    void setFramebufferSizeCallback(std::function<void(int, int)> callback) {
        m_framebufferSizeCallback = callback;
    }
    void setCursorPosCallback(std::function<void(double, double)> callback) {
        m_cursorPosCallback = callback;
    }

private:
    GlfwWindowPtr m_window;
    std::string m_title;
    int m_width;
    int m_height;

    // コールバック関数オブジェクト
    std::function<void(int, int)> m_framebufferSizeCallback;
    std::function<void(double, double)> m_cursorPosCallback;

    // GLFWコールバックのための静的ラッパー関数
    static void staticFramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void staticCursorPosCallback(GLFWwindow* window, double xpos, double ypos);
};

#endif // WINDOW_CONTEXT_HPP