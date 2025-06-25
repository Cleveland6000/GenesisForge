#include "renderer.hpp"
#include <iostream>
#include <iomanip> // std::fixed, std::setprecision のために必要
#include <sstream> // std::stringstream のために必要

Renderer::Renderer()
    : m_VAO(0), m_VBO(0), m_EBO(0), m_shaderProgram(0), m_textRenderer()
{
}

Renderer::~Renderer()
{
    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_VBO);
    glDeleteBuffers(1, &m_EBO);
    glDeleteProgram(m_shaderProgram);
}

bool Renderer::initialize(const FontData &fontData)
{
    // GLADはApplication::initialize()でロード済みなのでここでは不要

    glEnable(GL_DEPTH_TEST); // 深度テストはレンダリングに必要なのでRendererで有効化

    // --- 立方体の頂点データと色データ (共有頂点に修正) ---
    float vertices[] = {
        // 位置 (XYZ)                          色 (RGB)
        // 0: 右上奥
        0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, // 赤
        // 1: 右下奥
        0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, // 緑
        // 2: 左下奥
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, // 青
        // 3: 左上奥
        -0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, // 黄

        // 4: 右上手前
        0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 1.0f, // シアン
        // 5: 右下手前
        0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 1.0f, // マゼンタ
        // 6: 左下手前
        -0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, // 灰色
        // 7: 左上手前
        -0.5f, 0.5f, 0.5f, 0.8f, 0.2f, 0.6f // 紫
    };

    // インデックスデータ (修正)
    unsigned int indices[] = {
        // 奥の面 (2, 1, 0, 3)
        2, 1, 0,
        0, 3, 2,

        // 手前の面 (6, 5, 4, 7)
        6, 5, 4,
        4, 7, 6,

        // 左の面 (7, 3, 2, 6)
        7, 3, 2,
        2, 6, 7,

        // 右の面 (0, 1, 5, 4)
        0, 1, 5,
        5, 4, 0,

        // 上の面 (3, 0, 4, 7)
        3, 0, 4,
        4, 7, 3,

        // 下の面 (2, 6, 5, 1)
        2, 6, 5,
        5, 1, 2};

    // VAO, VBO, EBO生成とデータ転送
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);
    glBindVertexArray(m_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // 頂点属性ポインタを設定 (位置と色)
    // 位置 (layout = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    // 色 (layout = 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0); // VAOアンバインド

    // シェーダープログラムの作成
    m_shaderProgram = createShaderProgram("../shaders/basic.vert", "../shaders/basic.frag");
    if (m_shaderProgram == 0)
    {
        std::cerr << "Failed to create shader program for Renderer\n";
        return false;
    }

    // FontDataをメンバ変数にコピー (TextRenderer初期化用)
    m_fontData = fontData;

    // TextRendererの初期化
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

void Renderer::renderScene(const glm::mat4 &projection, const glm::mat4 &view, const Chunk &chunk, float cubeSpacing)
{
    glUseProgram(m_shaderProgram);

    // 投影行列とビュー行列をシェーダーに送る
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

    glBindVertexArray(m_VAO); // 立方体描画用にVAOをバインド

    int chunkSize = chunk.getSize();

    for (int x = 0; x < chunkSize; ++x)
    {
        for (int y = 0; y < chunkSize; ++y)
        {
            for (int z = 0; z < chunkSize; ++z)
            {
                if (chunk.getVoxel(x, y, z))
                { // trueの場合のみ立方体を描画
                    glm::mat4 model = glm::mat4(1.0f);
                    glm::vec3 cubeWorldPos = glm::vec3(x * cubeSpacing, y * cubeSpacing, z * cubeSpacing);
                    model = glm::translate(model, cubeWorldPos);

                    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

                    // 立方体を描画
                    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
                }
            }
        }
    }
    glBindVertexArray(0); // 描画終了後にVAOをアンバインド
}

void Renderer::renderOverlay(int screenWidth, int screenHeight, const std::string &fpsString, const std::string &positionString)
{
    // テキストレンダリングのための正射影行列
    glm::mat4 orthoProjection = glm::ortho(0.0f, static_cast<float>(screenWidth), 0.0f, static_cast<float>(screenHeight));

    const float TARGET_TEXT_HEIGHT_PX = 30.0f; // 例: 30ピクセル
    float textScale = 1.0f;
    if (m_fontData.baseFontSize > 0)
    {
        textScale = TARGET_TEXT_HEIGHT_PX / static_cast<float>(m_fontData.baseFontSize);
    }

    // FPS表示
    m_textRenderer.renderText(fpsString, 10.0f, static_cast<float>(screenHeight) - 60.0f, textScale, glm::vec3(1.0f, 1.0f, 1.0f), orthoProjection);

    // 座標表示 (FPSの下に配置)
    m_textRenderer.renderText(positionString,
                              10.0f,
                              (static_cast<float>(screenHeight) - 60.0f) - (TARGET_TEXT_HEIGHT_PX + 10.0f),
                              textScale,
                              glm::vec3(1.0f, 1.0f, 1.0f),
                              orthoProjection);
}

void Renderer::endFrame()
{
    // 特に何もなし（glSwapBuffersはApplication側で行うため）
}