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
#include "chunk.hpp"
#include "opengl_utils.hpp"

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
    void renderScene(const glm::mat4 &projection, const glm::mat4 &view, const Chunk &chunk, float cubeSpacing);
    void renderOverlay(int screenWidth, int screenHeight, const std::string &fpsString, const std::string &positionString);
    void endFrame();

private:
    GLuint m_VAO, m_VBO, m_EBO, m_shaderProgram;
    FontData m_fontData;
    TextRenderer m_textRenderer;
};

#endif // RENDERER_HPP
