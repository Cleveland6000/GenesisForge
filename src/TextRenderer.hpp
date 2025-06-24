#ifndef TEXT_RENDERER_HPP
#define TEXT_RENDERER_HPP

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include "FontLoader.hpp" // FontData の定義のために必要

// Note: createTextShaderProgram は TextRenderer.cpp 内に実装します。
// TextRenderer の初期化時にシェーダーをコンパイル＆リンクするためです。
// もしこの関数を他の場所でも使う場合は、opengl_utils.hpp に移動することを検討してください。

class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();

    // テキストレンダラーを初期化する関数
    // textShaderVertPath: テキスト描画用頂点シェーダーのパス
    // textShaderFragPath: テキスト描画用フラグメントシェーダーのパス
    // fontData: ロード済みのフォントデータへの参照
    bool initialize(const std::string& textShaderVertPath, const std::string& textShaderFragPath, const FontData& fontData);

    // テキストを描画する関数
    // text: 描画する文字列
    // x, y: 描画開始座標 (スクリーン座標)
    // scale: テキストのスケール
    // color: テキストの色 (RGB)
    // projection: 2D描画用の正射影行列 (glm::orthoで生成)
    void renderText(const std::string& text, float x, float y, float scale, const glm::vec3& color, const glm::mat4& projection);

private:
    GLuint m_textVAO;         // Vertex Array Object
    GLuint m_textVBO;         // Vertex Buffer Object
    GLuint m_textShaderProgram; // テキスト描画用シェーダープログラム

    const FontData* m_fontData; // 使用するフォントデータへのポインタ
};

#endif // TEXT_RENDERER_HPP