#include "renderer.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include "cube_data.hpp" // !!!ここを追加!!!
struct ChunkRenderData
{
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
    GLsizei indexCount = 0; // このチャンクメッシュのインデックス数

    // チャンクのモデル行列（ワールド空間でのチャンクの位置を定義）
    // 後でこの行列を使って、チャンク全体を移動させます。
    glm::mat4 modelMatrix = glm::mat4(1.0f);

    // コンストラクタで初期化とデストラクタで解放を行うことで、
    // ChunkRenderDataオブジェクトの寿命とOpenGLリソースの寿命を紐付けます。
    ChunkRenderData() = default; // デフォルトコンストラクタ

    // OpenGLリソースの解放
    ~ChunkRenderData()
    {
        if (VAO != 0)
            glDeleteVertexArrays(1, &VAO);
        if (VBO != 0)
            glDeleteBuffers(1, &VBO);
        if (EBO != 0)
            glDeleteBuffers(1, &EBO);
    }

    // コピーは禁止し、ムーブは許可する（パフォーマンスと安全のため）
    ChunkRenderData(const ChunkRenderData &) = delete;
    ChunkRenderData &operator=(const ChunkRenderData &) = delete;
    ChunkRenderData(ChunkRenderData &&other) noexcept
        : VAO(other.VAO), VBO(other.VBO), EBO(other.EBO), indexCount(other.indexCount), modelMatrix(other.modelMatrix)
    {
        other.VAO = 0; // 移動元を無効化
        other.VBO = 0;
        other.EBO = 0;
        other.indexCount = 0;
    }
    ChunkRenderData &operator=(ChunkRenderData &&other) noexcept
    {
        if (this != &other)
        {
            // 既存のリソースを解放
            if (VAO != 0)
                glDeleteVertexArrays(1, &VAO);
            if (VBO != 0)
                glDeleteBuffers(1, &VBO);
            if (EBO != 0)
                glDeleteBuffers(1, &EBO);

            // リソースを移動
            VAO = other.VAO;
            VBO = other.VBO;
            EBO = other.EBO;
            indexCount = other.indexCount;
            modelMatrix = other.modelMatrix;

            // 移動元を無効化
            other.VAO = 0;
            other.VBO = 0;
            other.EBO = 0;
            other.indexCount = 0;
        }
        return *this;
    }
};
Renderer::Renderer() : m_VAO(0), m_VBO(0), m_EBO(0), m_shaderProgram(0), m_textRenderer() {}

Renderer::~Renderer()
{
    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_VBO);
    glDeleteBuffers(1, &m_EBO);
    glDeleteProgram(m_shaderProgram);
}

bool Renderer::initialize(const FontData &fontData)
{
    glEnable(GL_DEPTH_TEST);

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, CUBE_VERTICES_SIZE, CUBE_VERTICES, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, CUBE_INDICES_SIZE, CUBE_INDICES, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

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

void Renderer::renderScene(const glm::mat4 &projection, const glm::mat4 &view, const Chunk &chunk, float cubeSpacing)
{
    glUseProgram(m_shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glBindVertexArray(m_VAO);

    int chunkSize = chunk.getSize();
    for (int x = 0; x < chunkSize; ++x)
    {
        for (int y = 0; y < chunkSize; ++y)
        {
            for (int z = 0; z < chunkSize; ++z)
            {
                if (chunk.getVoxel(x, y, z))
                {
                    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x * cubeSpacing, y * cubeSpacing, z * cubeSpacing));
                    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

                    glDrawElements(GL_TRIANGLES, CUBE_INDEX_COUNT, GL_UNSIGNED_INT, 0);
                }
            }
        }
    }
    glBindVertexArray(0);
}

void Renderer::renderOverlay(int screenWidth, int screenHeight, const std::string &fpsString, const std::string &positionString)
{
    glm::mat4 orthoProjection = glm::ortho(0.0f, (float)screenWidth, 0.0f, (float)screenHeight);
    const float TARGET_TEXT_HEIGHT_PX = 30.0f;
    float textScale = m_fontData.baseFontSize > 0 ? TARGET_TEXT_HEIGHT_PX / (float)m_fontData.baseFontSize : 1.0f;
    m_textRenderer.renderText(fpsString, 10.0f, (float)screenHeight - 60.0f, textScale, glm::vec3(1.0f), orthoProjection);
    m_textRenderer.renderText(positionString, 10.0f, (float)screenHeight - 60.0f - (TARGET_TEXT_HEIGHT_PX + 10.0f), textScale, glm::vec3(1.0f), orthoProjection);
}

void Renderer::endFrame() {}