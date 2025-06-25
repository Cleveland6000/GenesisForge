// game_application.hpp
#ifndef GAME_APPLICATION_HPP
#define GAME_APPLICATION_HPP

#include <memory>
#include <iostream>

#include "application.hpp"
#include "camera.hpp"
#include "timer.hpp"
#include "input_manager.hpp"
#include "fullscreen_manager.hpp"
#include "chunk.hpp"
#include "FontLoader.hpp"
#include "renderer.hpp"

class GameApplication
{
public:
    GameApplication();
    GameApplication(const GameApplication &) = delete;
    GameApplication &operator=(const GameApplication &) = delete;

    int run();
    bool setupDependencies();

private:
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<Timer> m_timer;
    std::unique_ptr<InputManager> m_inputManager;
    std::unique_ptr<FullscreenManager> m_fullscreenManager;
    std::unique_ptr<FontLoader> m_fontLoader;

    // FontData を unique_ptr で管理するように変更
    std::unique_ptr<FontData> m_fontData;

    std::unique_ptr<Chunk> m_chunk;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<Application> m_app;

    static constexpr int CHUNK_GRID_SIZE = 16;
};

#endif // GAME_APPLICATION_HPP