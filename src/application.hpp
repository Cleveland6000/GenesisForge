#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <array>
#include <memory>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "FontLoader.hpp"
#include "TextRenderer.hpp"
#include "camera.hpp"
#include "chunk_manager.hpp"
#include "input_manager.hpp"
#include "renderer.hpp"
#include "time/timer.hpp"
#include "window_context.hpp"

// Represents a plane in the form Ax + By + Cz + D = 0
struct Plane
{
    glm::vec3 normal;
    float distance; // Distance from origin
};

class Application
{
public:
    Application();
    ~Application();
    bool initialize();
    void run();

private:
    // Constants for window size and clear color
    static constexpr int INITIAL_SCR_WIDTH = 800;  // 新しい定数
    static constexpr int INITIAL_SCR_HEIGHT = 600; // 新しい定数
    static constexpr float CLEAR_COLOR_R = 0.0f;
    static constexpr float CLEAR_COLOR_G = 0.0f;
    static constexpr float CLEAR_COLOR_B = 0.0f;
    static constexpr float CLEAR_COLOR_A = 1.0f;

    // Unique pointers for managing resources
    std::unique_ptr<WindowContext> m_windowContext;
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<Timer> m_timer;
    std::unique_ptr<InputManager> m_inputManager;
    std::unique_ptr<FontData> m_fontData;
    std::unique_ptr<ChunkManager> m_chunkManager;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<FontLoader> m_fontLoader;

    // Projection matrix and overlay strings
    glm::mat4 m_projectionMatrix;
    std::string m_fpsString;
    std::string m_positionString;

    // World generation constants
    static constexpr int CHUNK_GRID_SIZE = 16;
    static constexpr float NOISE_SCALE = 0.006f;
    static constexpr int RENDER_DISTANCE_CHUNKS = 8;
    static constexpr unsigned int WORLD_SEED = 12345;
    static constexpr int WORLD_MAX_HEIGHT = 24;
    static constexpr int GROUND_LEVEL = 0;
    static constexpr int TERRAIN_OCTAVES = 4;
    static constexpr float TERRAIN_LACUNARITY = 2.0f;
    static constexpr float TERRAIN_PERSISTENCE = 0.5f;

    // Frustum culling planes
    std::array<Plane, 6> m_frustumPlanes;

    // Private methods
    void setupCallbacks();
    bool setupDependenciesAndLoadResources();
    void processInput();
    void update();
    void updateFpsAndPositionStrings();
    void render();
    void updateProjectionMatrix(int width, int height);

    // Frustum culling methods
    void extractFrustumPlanes(const glm::mat4 &viewProjection);
    bool isChunkInFrustum(const glm::ivec3 &chunkCoord) const;
};

#endif // APPLICATION_HPP