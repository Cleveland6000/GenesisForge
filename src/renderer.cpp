#include "renderer.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
// stb_image のインクルード
// STB_IMAGE_IMPLEMENTATION はプロジェクト内で1つの.cppファイルでのみ定義してください
// 今回はFontLoader.cppなど別のファイルで定義されているか、新しいファイルで定義することを想定し、ここでは定義しません。
#include "../dependencies/include/stb_image.hpp" // stb_image.h への正しいパスを設定してください

Renderer::Renderer() : m_shaderProgram(0), m_textRenderer(), m_textureID(0) {} // m_textureIDを初期化

Renderer::~Renderer()
{
    glDeleteProgram(m_shaderProgram);
    if (m_textureID != 0) {
        glDeleteTextures(1, &m_textureID); // テクスチャを解放
    }
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
    m_fontData = fontData;
    if (!m_textRenderer.initialize("../shaders/text.vert", "../shaders/text.frag", m_fontData))
    {
        std::cerr << "Failed to initialize TextRenderer in Renderer.\n";
        return false;
    }

    // テクスチャをロード
    // 実行ファイルからの相対パスを確認してください
    if (!loadTexture("../textures/my_block_texture.png")) {
        std::cerr << "Failed to load block texture.\n";
        return false;
    }

    // ロードしたテクスチャをシェーダーのuniformに設定
    glUseProgram(m_shaderProgram);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "ourTexture"), 0); // テクスチャユニット0を使用
    glUseProgram(0);

    return true;
}

// テクスチャをロードする関数の実装
bool Renderer::loadTexture(const std::string& path) {
    glGenTextures(1, &m_textureID);
    glBindTexture(GL_TEXTURE_2D, m_textureID);

    // テクスチャパラメータの設定 (テクスチャがピクセルアート風になるようにNEARESTを設定)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int width, height, nrChannels;
    // stb_image を使用して画像をロード
    unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
    if (data)
    {
        GLenum format = GL_RGB;
        if (nrChannels == 4) {
            format = GL_RGBA;
        } else if (nrChannels == 3) {
            format = GL_RGB;
        }
        // テクスチャをOpenGLに転送
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D); // ミップマップを生成 (NEARESTフィルターでは必須ではありませんが、良い習慣です)
        stbi_image_free(data); // 画像データを解放
        return true;
    }
    else
    {
        std::cerr << "Failed to load texture at path: " << path << std::endl;
        // ロード失敗時でもdataがNULLでない場合があるため、stbi_image_freeを呼ぶ
        if (data) stbi_image_free(data);
        return false;
    }
}

void Renderer::beginFrame(const glm::vec4 &clearColor)
{
    glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::renderScene(const glm::mat4 &projection, const glm::mat4 &view, const ChunkRenderData &chunkRenderData, const glm::mat4 &model)
{
    if (chunkRenderData.VAO == 0 || chunkRenderData.indexCount == 0)
    {
        return;
    }

    glUseProgram(m_shaderProgram);

    // テクスチャユニット0をアクティブにし、テクスチャをバインド
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureID);

    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(chunkRenderData.VAO);
    glDrawElements(GL_TRIANGLES, chunkRenderData.indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glBindTexture(GL_TEXTURE_2D, 0); // テクスチャのバインドを解除
}

void Renderer::renderOverlay(int screenWidth, int screenHeight, const std::string &fpsString, const std::string &positionString)
{
    glm::mat4 orthoProjection = glm::ortho(0.0f, (float)screenWidth, 0.0f, (float)screenHeight);

    const float RELATIVE_TEXT_HEIGHT_RATIO = 1.0f / 15.0f;
    float targetTextHeightPx = screenHeight * RELATIVE_TEXT_HEIGHT_RATIO;

    float textScale = m_fontData.baseFontSize > 0 ? targetTextHeightPx / (float)m_fontData.baseFontSize : 1.0f;

    float margin = screenHeight * 0.02f;

    m_textRenderer.renderText(fpsString, margin, (float)screenHeight - targetTextHeightPx - margin, textScale, glm::vec3(1.0f), orthoProjection);
    m_textRenderer.renderText(positionString, margin, (float)screenHeight - (targetTextHeightPx * 2) - (margin * 2), textScale, glm::vec3(1.0f), orthoProjection);
}

void Renderer::endFrame() {
}