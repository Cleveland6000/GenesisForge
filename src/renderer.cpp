#include "renderer.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include "chunk_mesh_generator.hpp" // これのみ残す


// コンストラクタは変更なし
Renderer::Renderer() : m_shaderProgram(0), m_textRenderer() {}

// デストラクタは変更なし
Renderer::~Renderer()
{
    glDeleteProgram(m_shaderProgram);
}

// initialize は変更なし
bool Renderer::initialize(const FontData &fontData)
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE); 

    m_shaderProgram = createShaderProgram("../shaders/basic.vert", "../shaders/basic.frag");
    if (m_shaderProgram == 0)
    {
        std::cerr << "Failed to create shader program for Renderer\n";
        return false;
    }
    m_fontData = fontData;
    if (!m_textRenderer.initialize("../shaders/text.vert", "../shaders/text.frag", m_fontData))
    {
        std::cerr << "Failed to initialize TextRenderer in Renderer.\n";
        return false;
    }
    return true;
}

void Renderer::beginFrame(const glm::vec4 &clearColor)
{
    glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::renderScene(const glm::mat4 &projection, const glm::mat4 &view, const ChunkRenderData &chunkRenderData)
{
    if (chunkRenderData.VAO == 0 || chunkRenderData.indexCount == 0) {
        return;
    }

    glUseProgram(m_shaderProgram); 
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(chunkRenderData.modelMatrix));

    glBindVertexArray(chunkRenderData.VAO);
    glDrawElements(GL_TRIANGLES, chunkRenderData.indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0); 
}


void Renderer::renderOverlay(int screenWidth, int screenHeight, const std::string &fpsString, const std::string &positionString)
{
    glm::mat4 orthoProjection = glm::ortho(0.0f, (float)screenWidth, 0.0f, (float)screenHeight);
    const float TARGET_TEXT_HEIGHT_PX = 60.0f;
    float textScale = m_fontData.baseFontSize > 0 ? TARGET_TEXT_HEIGHT_PX / (float)m_fontData.baseFontSize : 1.0f;
    m_textRenderer.renderText(fpsString, 10.0f, (float)screenHeight - 60.0f, textScale, glm::vec3(1.0f), orthoProjection);
    m_textRenderer.renderText(positionString, 10.0f, (float)screenHeight - 60.0f - (TARGET_TEXT_HEIGHT_PX + 10.0f), textScale, glm::vec3(1.0f), orthoProjection);
}

void Renderer::endFrame() {}