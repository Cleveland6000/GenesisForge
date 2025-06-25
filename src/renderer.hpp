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
#include "opengl_utils.hpp" // createShaderProgram関数のために必要

// レンダラーに渡すボクセルの描画に必要な情報構造体
struct VoxelRenderInfo
{
    glm::ivec3 position; // グリッド座標
    // 必要であれば、ここに色やテクスチャ情報などを追加できます
};

class Renderer
{
public:
    Renderer();
    ~Renderer();

    // 初期化
    bool initialize(const FontData &fontData);
    // レンダリング開始（フレームの準備）
    void beginFrame(const glm::vec4 &clearColor);
    // 3Dシーンのレンダリング
    void renderScene(const glm::mat4 &projection, const glm::mat4 &view, const Chunk &chunk, float cubeSpacing);
    // 2Dオーバーレイ（テキストなど）のレンダリング
    void renderOverlay(int screenWidth, int screenHeight, const std::string &fpsString, const std::string &positionString);
    // レンダリング終了（バッファスワップなどはApplicationで行う）
    void endFrame();

private:
    GLuint m_VAO;
    GLuint m_VBO;
    GLuint m_EBO;
    GLuint m_shaderProgram;

    FontData m_fontData; // FontLoaderから受け取ったフォントデータ
    TextRenderer m_textRenderer;
};

#endif // RENDERER_HPP