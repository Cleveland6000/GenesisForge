#ifndef FULLSCREEN_MANAGER_HPP
#define FULLSCREEN_MANAGER_HPP

// GLFWがgl.hをインクルードするのを防ぐために、gladの前に定義
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h> // gladがないのでここはそのままでも問題ないが、他の場所との一貫性のために追加
#include <iostream>

class FullscreenManager
{
public:
    FullscreenManager();

    // フルスクリーン状態をトグルする関数
    // window: 対象のGLFWウィンドウ
    // width, height: 現在のフレームバッファサイズ（updateProjectionMatrix呼び出しのため）
    void toggleFullscreen(GLFWwindow *window, int currentWidth, int currentHeight);

    // フルスクリーン状態を取得
    bool isFullscreen() const { return m_isFullscreen; }

private:
    bool m_isFullscreen;
    int m_windowedX, m_windowedY;
    int m_windowedWidth, m_windowedHeight;

    // フレームバッファサイズ変更をApplicationクラスに通知するためのコールバックポインタ
    // または、Applicationクラスに依存しない形にする場合は、このマネージャーが直接Viewportを設定するなどの方針になる
    // 今回はApplicationクラスのupdateProjectionMatrixを呼び出す想定で、
    // Applicationインスタンスへのポインタをユーザーポインタから取得する形で対応します。
};

#endif // FULLSCREEN_MANAGER_HPP