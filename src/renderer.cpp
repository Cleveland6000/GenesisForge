#include "renderer.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
// #include "chunk_mesh_generator.hpp" // Renderer がメッシュ生成に直接関与しないため、通常は不要です。

Renderer::Renderer() : m_shaderProgram(0), m_textRenderer() {}

Renderer::~Renderer()
{
    glDeleteProgram(m_shaderProgram);
}

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
    m_fontData = fontData; // FontDataをコピー
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

// renderScene のシグネチャに model 行列を追加
void Renderer::renderScene(const glm::mat4 &projection, const glm::mat4 &view, const ChunkRenderData &chunkRenderData, const glm::mat4 &model)
{
    // VAO と indexCount が有効であるかチェック
    if (chunkRenderData.VAO == 0 || chunkRenderData.indexCount == 0)
    {
        return;
    }

    glUseProgram(m_shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    // ここで渡された model 行列を使用
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(chunkRenderData.VAO);
    glDrawElements(GL_TRIANGLES, chunkRenderData.indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Renderer::renderOverlay(int screenWidth, int screenHeight, const std::string &fpsString, const std::string &positionString)
{
    glm::mat4 orthoProjection = glm::ortho(0.0f, (float)screenWidth, 0.0f, (float)screenHeight);

    // 画面の高さに対する比率でテキストの高さを決定
    // 例: 画面の高さの約1/15の高さになるように調整
    const float RELATIVE_TEXT_HEIGHT_RATIO = 1.0f / 15.0f; 
    float targetTextHeightPx = screenHeight * RELATIVE_TEXT_HEIGHT_RATIO; // ウィンドウの高さに基づいて計算

    float textScale = m_fontData.baseFontSize > 0 ? targetTextHeightPx / (float)m_fontData.baseFontSize : 1.0f;

    // HUDテキストのレンダリング
    // Y座標は画面の高さからテキストの高さとマージンを引いて計算
    // マージンも画面の高さに連動させるか、相対的な値にするのが望ましい
    float margin = screenHeight * 0.02f; // 例: 画面の高さの2%をマージンとする

    m_textRenderer.renderText(fpsString, margin, (float)screenHeight - targetTextHeightPx - margin, textScale, glm::vec3(1.0f), orthoProjection);
    m_textRenderer.renderText(positionString, margin, (float)screenHeight - (targetTextHeightPx * 2) - (margin * 2), textScale, glm::vec3(1.0f), orthoProjection);
}

void Renderer::endFrame() {
    // 必要であればここで OpenGL の状態をリセットしたり、追加のクリーンアップを行う
}
