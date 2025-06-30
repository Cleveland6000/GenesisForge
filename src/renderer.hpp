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
#include "opengl_utils.hpp" // createShaderProgram などが定義されていると仮定

struct ChunkRenderData {
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
    GLsizei indexCount = 0;

    ChunkRenderData() = default;
    ~ChunkRenderData() {
        if (VAO != 0) glDeleteVertexArrays(1, &VAO);
        if (VBO != 0) glDeleteBuffers(1, &VBO);
        if (EBO != 0) glDeleteBuffers(1, &EBO);
    }
    ChunkRenderData(const ChunkRenderData&) = delete;
    ChunkRenderData& operator=(const ChunkRenderData&) = delete;
    ChunkRenderData(ChunkRenderData&& other) noexcept
        : VAO(other.VAO), VBO(other.VBO), EBO(other.EBO), indexCount(other.indexCount) {
        other.VAO = 0;
        other.VBO = 0;
        other.EBO = 0;
        other.indexCount = 0;
    }
    ChunkRenderData& operator=(ChunkRenderData&& other) noexcept {
        if (this != &other) {
            if (VAO != 0) glDeleteVertexArrays(1, &VAO);
            if (VBO != 0) glDeleteBuffers(1, &VBO);
            if (EBO != 0) glDeleteBuffers(1, &EBO);
            VAO = other.VAO;
            VBO = other.VBO;
            EBO = other.EBO;
            indexCount = other.indexCount;
            other.VAO = 0;
            other.VBO = 0;
            other.EBO = 0;
            other.indexCount = 0;
        }
        return *this;
    }
};

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
    void renderScene(const glm::mat4 &projection, const glm::mat4 &view, const ChunkRenderData &chunkRenderData, const glm::mat4 &model);
    void renderOverlay(int screenWidth, int screenHeight, const std::string &fpsString, const std::string &positionString);
    void endFrame();

    // フォグパラメータを設定する新しいメソッド
    void setFogParameters(const glm::vec3& color, float start, float end, float density);

private:
    GLuint m_shaderProgram;
    FontData m_fontData;
    TextRenderer m_textRenderer;
    GLuint m_textureID;
    bool loadTexture(const std::string& path);

    // フォグのuniformロケーション
    GLint m_fogColorLoc;
    GLint m_fogStartLoc;
    GLint m_fogEndLoc;
    GLint m_fogDensityLoc;
};

#endif // RENDERER_HPP
