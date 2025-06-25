#ifndef FONT_LOADER_HPP
#define FONT_LOADER_HPP

#include <string>
#include <map>
#include <glad/glad.h> // GLuint のために必要

// 各文字の情報を保持する構造体
struct CharInfo {
    int id;
    int x, y;
    int width, height;
    int xoffset, yoffset;
    int xadvance;
    // float page; // 必要であれば追加
    // float chnl; // 必要であれば追加
};

// フォント全体に関する情報を保持する構造体
struct FontData {
    int lineHeight;   // 行の高さ (JSONのcommon.lineHeight)
    int baseFontSize; // フォント生成時の基準フォントサイズ (JSONのinfo.size)
    int textureWidth; // フォントアトラスの幅
    int textureHeight; // フォントアトラスの高さ
    GLuint textureID; // OpenGLテクスチャID
    std::map<int, CharInfo> chars; // 文字IDごとの情報
    bool isLoaded = false; // フォントが正常にロードされたか
};

class FontLoader {
public:
    FontLoader();
    ~FontLoader();

    // SDFフォントアトラスとJSONメタデータをロードする関数
    bool loadSDFont(const std::string& fontJsonPath, const std::string& fontPngPath, FontData& fontData);

private:
    // PNG画像をロードしてOpenGLテクスチャを作成するヘルパー関数
    GLuint loadTexture(const std::string& imagePath, int& width, int& height);
};

#endif // FONT_LOADER_HPP
