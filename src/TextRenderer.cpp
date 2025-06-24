#include "TextRenderer.hpp"
#include <iostream>
#include <vector>
#include <glm/gtc/matrix_transform.hpp> // for glm::ortho
#include <glm/gtc/type_ptr.hpp>         // for glm::value_ptr

// ApplicationクラスのcreateShaderProgram関数を再利用するためにインクルード
// もしcreateShaderProgramがグローバル関数でなければ、このファイルにコピーするか、
// 独立したUtil関数として作成し直す必要があります。
// 現状のApplication.hpp/cppからcreateShaderProgramがアクセスできない場合を想定し、
// ここに直接createShaderProgramの実装を含めます。
// 通常はopengl_utils.hppのような共通のヘッダーに移動します。
#include <fstream>
#include <sstream>

// シェーダーファイルを読み込み、コンパイルし、リンクしてシェーダープログラムを作成する関数
GLuint createTextShaderProgram(const std::string &vertexPath, const std::string &fragmentPath)
{
    // 1. シェーダーソースコードをファイルから取得
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;

    // ifstreamオブジェクトが例外をスローできるように設定
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
        // ファイルを開く
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;
        // ファイルの内容をストリームに読み込む
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        // ファイルを閉じる
        vShaderFile.close();
        fShaderFile.close();
        // ストリームの内容を文字列に変換
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    }
    catch (std::ifstream::failure e)
    {
        std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " << e.what() << std::endl;
        std::cerr << "Vertex Shader Path: " << vertexPath << std::endl;
        std::cerr << "Fragment Shader Path: " << fragmentPath << std::endl;
        return 0;
    }
    const char *vShaderCode = vertexCode.c_str();
    const char *fShaderCode = fragmentCode.c_str();

    // 2. シェーダーをコンパイル
    GLuint vertex, fragment;
    int success;
    char infoLog[512];

    // 頂点シェーダー
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
        return 0;
    };

    // フラグメントシェーダー
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
        return 0;
    };

    // シェーダープログラム
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertex);
    glAttachShader(shaderProgram, fragment);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        GLenum error = glGetError();
        if (error != GL_NO_ERROR)
        {
            std::cerr << "OpenGL Error after shader program creation/linking: " << error << std::endl;
            // エラーコードに応じて、さらに詳細なログ出力や処理を行うこともできます
        }
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
                  << infoLog << std::endl;
        return 0;
    }

    // シェーダーを削除 (もうプログラムにリンクされたので不要)
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return shaderProgram;
}

// コンストラクタ
TextRenderer::TextRenderer()
    : m_textVAO(0), m_textVBO(0), m_textShaderProgram(0), m_fontData(nullptr)
{
}

// デストラクタ
TextRenderer::~TextRenderer()
{
    if (m_textVAO != 0)
        glDeleteVertexArrays(1, &m_textVAO);
    if (m_textVBO != 0)
        glDeleteBuffers(1, &m_textVBO);
    if (m_textShaderProgram != 0)
        glDeleteProgram(m_textShaderProgram);
    // フォントテクスチャはFontDataが所有しているのでここでは解放しない
}

// TextRendererを初期化する関数
bool TextRenderer::initialize(const std::string &textShaderVertPath, const std::string &textShaderFragPath, const FontData &fontData)
{
    m_fontData = &fontData; // フォントデータへの参照を保存

    // シェーダープログラムの作成
    m_textShaderProgram = createTextShaderProgram(textShaderVertPath, textShaderFragPath);
    if (m_textShaderProgram == 0)
    {
        std::cerr << "Error: Failed to create text shader program." << std::endl;
        return false;
    }

    // VAOとVBOのセットアップ
    glGenVertexArrays(1, &m_textVAO);
    glGenBuffers(1, &m_textVBO);

    glBindVertexArray(m_textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_textVBO);

    // 頂点属性ポインタを設定
    // レイアウト0: 位置 (vec2)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void *)0);
    glEnableVertexAttribArray(0);
    // レイアウト1: UV座標 (vec2)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void *)(sizeof(float) * 2));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return true;
}

// テキストを描画する関数
void TextRenderer::renderText(const std::string &text, float x, float y, float scale, const glm::vec3 &color, const glm::mat4 &projection)
{
    if (!m_fontData || !m_fontData->isLoaded || m_textShaderProgram == 0)
    {
        std::cerr << "Warning: TextRenderer not initialized or font not loaded." << std::endl;
        return;
    }

    glUseProgram(m_textShaderProgram);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_fontData->textureID);
    glUniform1i(glGetUniformLocation(m_textShaderProgram, "fontAtlas"), 0); // テクスチャユニット0

    glUniformMatrix4fv(glGetUniformLocation(m_textShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(glGetUniformLocation(m_textShaderProgram, "textColor"), 1, glm::value_ptr(color));
    // ここでoutlineColor, outlineWidthなどをuniformで設定することも可能

    glBindVertexArray(m_textVAO);

    // ブレンディングを有効化 (テキストの透明な部分を描画しないため)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST); // テキストは2Dオーバーレイなので深度テストは不要

    float currentX = x; // 現在のX座標
    float currentY = y; // 現在のY座標

    // 各文字の頂点データを格納するベクトル
    std::vector<float> vertices; // pos.x, pos.y, uv.x, uv.y

    for (char c_char : text)
    {
        int char_id = static_cast<int>(c_char);
        if (m_fontData->chars.count(char_id))
        {
            const CharInfo &charInfo = m_fontData->chars.at(char_id);

            // 各文字はクアッド (2つの三角形) で構成される
            // 頂点位置とUV座標を計算

            // 文字のワールド座標での幅と高さ
            float charWidth = (float)charInfo.width * scale;
            float charHeight = (float)charInfo.height * scale;

            // クアッドの左下隅のオフセット位置
            // BMFontのyoffsetは、ベースラインからグリフの最上部までのオフセット
            // OpenGLのY軸は通常上向きなので、yoffsetを考慮してY座標を調整
            float xpos = currentX + (float)charInfo.xoffset * scale;
            float ypos = currentY + (m_fontData->lineHeight - (float)charInfo.yoffset) * scale;
            // または、ypos = currentY + (float)charInfo.yoffset * scale; // これで合う場合もある

            // テクスチャUV座標 (正規化)
            float uvX = (float)charInfo.x / (float)m_fontData->textureWidth;
            float uvY = (float)charInfo.y / (float)m_fontData->textureHeight;
            float uvWidth = (float)charInfo.width / (float)m_fontData->textureWidth;
            float uvHeight = (float)charInfo.height / (float)m_fontData->textureHeight;

            // クアッドの頂点データ
            // 頂点データは左下、左上、右上、右下の順で格納する
            // 各頂点は (X, Y, U, V) の4つのfloat値を持つ
            // (xpos, ypos - charHeight)      左下 (pos.x, pos.y, uv.x, uv.y + uvHeight)
            // (xpos, ypos)                   左上 (pos.x, pos.y, uv.x, uv.y)
            // (xpos + charWidth, ypos)       右上 (pos.x + width, pos.y, uv.x + uvWidth, uv.y)
            // (xpos + charWidth, ypos - charHeight) 右下 (pos.x + width, pos.y - height, uv.x + uvWidth, uv.y + uvHeight)

            // 最初の三角形 (左上、左下、右下)
            vertices.push_back(xpos);
            vertices.push_back(ypos);
            vertices.push_back(uvX);
            vertices.push_back(uvY); // 左上
            vertices.push_back(xpos);
            vertices.push_back(ypos - charHeight);
            vertices.push_back(uvX);
            vertices.push_back(uvY + uvHeight); // 左下
            vertices.push_back(xpos + charWidth);
            vertices.push_back(ypos - charHeight);
            vertices.push_back(uvX + uvWidth);
            vertices.push_back(uvY + uvHeight); // 右下

            // 2番目の三角形 (左上、右下、右上)
            vertices.push_back(xpos);
            vertices.push_back(ypos);
            vertices.push_back(uvX);
            vertices.push_back(uvY); // 左上
            vertices.push_back(xpos + charWidth);
            vertices.push_back(ypos - charHeight);
            vertices.push_back(uvX + uvWidth);
            vertices.push_back(uvY + uvHeight); // 右下
            vertices.push_back(xpos + charWidth);
            vertices.push_back(ypos);
            vertices.push_back(uvX + uvWidth);
            vertices.push_back(uvY); // 右上

            // 次の文字のX座標を更新 (カーニングは未実装だがxadvanceを使う)
            currentX += (float)charInfo.xadvance * scale;
        }
    }

    // VBOにデータを転送
    glBindBuffer(GL_ARRAY_BUFFER, m_textVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_DYNAMIC_DRAW); // 動的なテキストなのでDYNAMIC_DRAW

    // 描画
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vertices.size() / 4); // 各頂点4つのfloat値

    // クリーンアップ
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST); // 深度テストを元に戻す
}