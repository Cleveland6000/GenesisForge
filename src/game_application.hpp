// src/game_application.hpp
#ifndef GAME_APPLICATION_HPP
#define GAME_APPLICATION_HPP


#include "application.hpp" // 既存のApplicationクラス
// ... 他の依存オブジェクトのヘッダー ...
#include "window_context.hpp" // ★WindowContextのヘッダーを追加★

#include <memory>
#include <iostream>
class GameApplication
{
public:
    GameApplication();
    ~GameApplication();
    GameApplication(const GameApplication &) = delete;            // コピー禁止
    GameApplication &operator=(const GameApplication &) = delete; // コピー代入禁止

    int run(); // アプリケーションのメイン実行ループ

private:
    // GameApplicationが依存オブジェクトと、それらをラップするApplicationインスタンスを所有
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<Timer> m_timer;
    std::unique_ptr<InputManager> m_inputManager;
    std::unique_ptr<FullscreenManager> m_fullscreenManager;
    std::unique_ptr<FontLoader> m_fontLoader;
    std::unique_ptr<FontData> m_fontData;
    std::unique_ptr<Chunk> m_chunk;
    std::unique_ptr<Renderer> m_renderer;

    std::unique_ptr<Application> m_app;

    // ★★★ここを追加★★★
    std::unique_ptr<WindowContext> m_windowContext; // WindowContextを所有するメンバー

    // 定数
    static constexpr int CHUNK_GRID_SIZE = 16;

    // 初期化ヘルパーメソッド
    bool setupDependencies();
};

#endif // GAME_APPLICATION_HPP