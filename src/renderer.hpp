#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp> // <- これを追加
#include <memory>
#include <string>
#include <vector>
#include "FontLoader.hpp"
#include "TextRenderer.hpp"
#include "opengl_utils.hpp"
#include "chunk_render_data.hpp" // ChunkRenderData の定義をインクルード

// VoxelRenderInfo は ChunkRenderData とは別のものなので、そのまま残します
struct VoxelRenderInfo {
    glm::ivec3 position;
};

class Renderer
{
public:
    Renderer();
    ~Renderer();
    bool initialize(const FontData &fontData);
    void beginFrame(const glm::vec4 &clearColor);
    // ChunkRenderData をパラメータに持つ renderScene を定義
    void renderScene(const glm::mat4 &projection, const glm::mat4 &view, const ChunkRenderData &chunkRenderData, const glm::mat4 &model);
    void renderOverlay(int screenWidth, int screenHeight, const std::string &fpsString, const std::string &positionString);
    void endFrame();

private:
    GLuint m_shaderProgram;
    FontData m_fontData;
    TextRenderer m_textRenderer;
    GLuint m_textureID;
    bool loadTexture(const std::string& path);
};

#endif // RENDERER_HPP