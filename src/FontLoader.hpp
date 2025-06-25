#ifndef FONT_LOADER_HPP
#define FONT_LOADER_HPP

#define GLFW_INCLUDE_NONE // これをGLFWの前に置く！
#include <glad/glad.h>   // これを一番最初に置く
#include <GLFW/glfw3.h> // glad.h の次に GLFW をインクルード


#include <string>
#include <map>
#include <iostream> // ★ここを追加★ std::cout のために必要

struct CharInfo
{
    int id, x, y, width, height, xoffset, yoffset, xadvance;
};

struct FontData
{
    int lineHeight = 0;
    int baseFontSize = 0;
    int textureWidth = 0;
    int textureHeight = 0;
    GLuint textureID = 0;
    std::map<int, CharInfo> chars;
    bool isLoaded = false;

    // FontData のデフォルトコンストラクタを明示的に定義
    FontData() : lineHeight(0), baseFontSize(0), textureWidth(0), textureHeight(0), textureID(0), isLoaded(false) {
        std::cout << "--- FontData default constructor called. ---\n";
    }
};

class FontLoader
{
public:
    FontLoader();
    ~FontLoader();
    bool loadSDFont(const std::string &fontJsonPath, const std::string &fontPngPath, FontData &fontData);

private:
    GLuint loadTexture(const std::string &imagePath, int &width, int &height);
};

#endif // FONT_LOADER_HPP