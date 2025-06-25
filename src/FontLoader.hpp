#ifndef FONT_LOADER_HPP
#define FONT_LOADER_HPP

#include <string>
#include <map>
#include <glad/glad.h>

struct CharInfo
{
    int id, x, y, width, height, xoffset, yoffset, xadvance;
};

struct FontData
{
    int lineHeight, baseFontSize, textureWidth, textureHeight;
    GLuint textureID;
    std::map<int, CharInfo> chars;
    bool isLoaded = false;
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
