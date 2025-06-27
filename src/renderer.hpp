#ifndef RENDERER_HPP
#define RENDERER_HPP
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>
#include <vector>
#include "FontLoader.hpp"
#include "TextRenderer.hpp"
#include "chunk/chunk.hpp"
#include "opengl_utils.hpp"
#include "chunk_mesh_generator.hpp"
struct ChunkRenderData {
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
    GLsizei indexCount = 0;
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    ChunkRenderData() = default;
    ~ChunkRenderData() {
        if (VAO != 0) glDeleteVertexArrays(1, &VAO);
        if (VBO != 0) glDeleteBuffers(1, &VBO);
        if (EBO != 0) glDeleteBuffers(1, &EBO);
    }
    ChunkRenderData(const ChunkRenderData&) = delete;
    ChunkRenderData& operator=(const ChunkRenderData&) = delete;
    ChunkRenderData(ChunkRenderData&& other) noexcept
        : VAO(other.VAO), VBO(other.VBO), EBO(other.EBO), indexCount(other.indexCount), modelMatrix(other.modelMatrix) {
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
            modelMatrix = other.modelMatrix;
            other.VAO = 0;
            other.VBO = 0;
            other.EBO = 0;
            other.indexCount = 0;
        }
        return *this;
    }
};
struct VoxelRenderInfo
{
    glm::ivec3 position;
};
class Renderer
{
public:
    Renderer();
    ~Renderer();
    bool initialize(const FontData &fontData);
    void beginFrame(const glm::vec4 &clearColor);
    void renderScene(const glm::mat4 &projection, const glm::mat4 &view, const ChunkRenderData &chunkRenderData);
    void renderOverlay(int screenWidth, int screenHeight, const std::string &fpsString, const std::string &positionString);
    void endFrame();
private:
    GLuint m_shaderProgram;
    FontData m_fontData;
    TextRenderer m_textRenderer;
};
#endif // RENDERER_HPP
