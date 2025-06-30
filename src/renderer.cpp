#include "renderer.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
// stb_image のインクルード
// STB_IMAGE_IMPLEMENTATION はプロジェクト内で1つの.cppファイルでのみ定義してください
// 今回はFontLoader.cppなど別のファイルで定義されているか、新しいファイルで定義することを想定し、ここでは定義しません。
#include "../dependencies/include/stb_image.hpp" // stb_image.h への正しいパスを設定してください

// opengl_utils.hpp に createShaderProgram が定義されていると仮定
// もし定義されていなければ、ここで実装するか、適切な場所に移動してください
extern GLuint createShaderProgram(const std::string& vertexPath, const std::string& fragmentPath);


Renderer::Renderer() : m_shaderProgram(0), m_textRenderer(), m_textureID(0),
                       m_fogColorLoc(-1), m_fogStartLoc(-1), m_fogEndLoc(-1), m_fogDensityLoc(-1) {}

Renderer::~Renderer()
{
    glDeleteProgram(m_shaderProgram);
    if (m_textureID != 0) {
        glDeleteTextures(1, &m_textureID);
    }
}

bool Renderer::initialize(const FontData &fontData)
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    
    // シェーダーパスを block_vertex_shader.glsl と block_fragment_shader.glsl に変更
    // 実行ファイルがbuildディレクトリにある場合、src/shaders/への相対パスは ../src/shaders/ となります。
    m_shaderProgram = createShaderProgram("../shaders/block_vertex_shader.glsl", "../shaders/block_fragment_shader.glsl");
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

    if (!loadTexture("../textures/my_block_texture.png")) {
        std::cerr << "Failed to load block texture.\n";
        return false;
    }

    glUseProgram(m_shaderProgram);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "ourTexture"), 0); // テクスチャユニット0を使用

    // ライティング関連のユニフォーム初期値を設定
    glUniform3f(glGetUniformLocation(m_shaderProgram, "lightDir"), 0.5f, -1.0f, 0.5f); // 光源の方向 (例: 右上奥から手前下)
    glUniform1f(glGetUniformLocation(m_shaderProgram, "ambientStrength"), 0.3f); // 環境光の強さ

    // フォグのユニフォームロケーションを取得
    m_fogColorLoc = glGetUniformLocation(m_shaderProgram, "fogColor");
    m_fogStartLoc = glGetUniformLocation(m_shaderProgram, "fogStart");
    m_fogEndLoc = glGetUniformLocation(m_shaderProgram, "fogEnd");
    m_fogDensityLoc = glGetUniformLocation(m_shaderProgram, "fogDensity");

    // デフォルトのフォグパラメータをシェーダーに設定 (初期化時)
    // Applicationクラスの初期値と一致させるか、ここでデフォルト値を設定
    glUniform3f(m_fogColorLoc, 0.5f, 0.5f, 0.7f); // 例: 少し青みがかったグレー
    glUniform1f(m_fogStartLoc, 50.0f);
    glUniform1f(m_fogEndLoc, 500.0f);
    glUniform1f(m_fogDensityLoc, 0.005f);

    glUseProgram(0);

    return true;
}

bool Renderer::loadTexture(const std::string& path) {
    glGenTextures(1, &m_textureID);
    glBindTexture(GL_TEXTURE_2D, m_textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int width, height, nrChannels;
    unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
    if (data)
    {
        GLenum format = GL_RGB;
        if (nrChannels == 4) {
            format = GL_RGBA;
        } else if (nrChannels == 3) {
            format = GL_RGB;
        }
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
        return true;
    }
    else
    {
        std::cerr << "Failed to load texture at path: " << path << std::endl;
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

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureID);

    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    // NormalMatrix を計算してシェーダーに渡す
    // モデル行列の左上3x3部分を抽出して逆転置
    glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(model)));
    glUniformMatrix3fv(glGetUniformLocation(m_shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));

    glBindVertexArray(chunkRenderData.VAO);
    glDrawElements(GL_TRIANGLES, chunkRenderData.indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::renderOverlay(int screenWidth, int screenHeight, const std::string &fpsString, const std::string &positionString)
{
    glm::mat4 orthoProjection = glm::ortho(0.0f, (float)screenWidth, 0.0f, (float)screenHeight);

    const float RELATIVE_TEXT_HEIGHT_RATIO = 1.0f / 20.0f;
    float targetTextHeightPx = screenHeight * RELATIVE_TEXT_HEIGHT_RATIO;

    float textScale = m_fontData.baseFontSize > 0 ? targetTextHeightPx / (float)m_fontData.baseFontSize : 1.0f;

    float margin = screenHeight * 0.02f;

    m_textRenderer.renderText(fpsString, margin, (float)screenHeight - targetTextHeightPx - margin, textScale, glm::vec3(1.0f), orthoProjection);
    m_textRenderer.renderText(positionString, margin, (float)screenHeight - (targetTextHeightPx * 2) - (margin * 2), textScale, glm::vec3(1.0f), orthoProjection);
}

void Renderer::endFrame() {
}

// フォグパラメータ設定メソッドの実装
void Renderer::setFogParameters(const glm::vec3& color, float start, float end, float density)
{
    // シェーダーが使用されていることを確認してからuniformを設定
    // renderScene内で既にglUseProgram(m_shaderProgram);が呼ばれていることを前提とする
    // もしrenderSceneの前にsetFogParametersが呼ばれる場合、ここでm_shaderProgram->use()が必要です。
    // 今回はApplication::render()の呼び出し順序に合わせて、renderSceneの直前に呼び出すため、
    // renderScene内でuse()されるm_shaderProgramがそのまま有効であると仮定します。
    glUseProgram(m_shaderProgram); // 念のためここでuse()を呼ぶのが安全

    glUniform3fv(m_fogColorLoc, 1, glm::value_ptr(color));
    glUniform1f(m_fogStartLoc, start);
    glUniform1f(m_fogEndLoc, end);
    glUniform1f(m_fogDensityLoc, density);
}
