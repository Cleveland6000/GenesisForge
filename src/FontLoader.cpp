#include "FontLoader.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <json.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.hpp>

FontLoader::FontLoader() {}
FontLoader::~FontLoader() {}

GLuint FontLoader::loadTexture(const std::string &imagePath, int &width, int &height)
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int nrChannels;
    unsigned char *data = stbi_load(imagePath.c_str(), &width, &height, &nrChannels, 0);
    if (data)
    {
        GLenum format = (nrChannels == 3) ? GL_RGB : (nrChannels == 1) ? GL_RED
                                                                       : GL_RGBA;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cerr << "Failed to load texture: " << imagePath << std::endl;
        stbi_image_free(data);
        return 0;
    }
    stbi_image_free(data);
    return textureID;
}

bool FontLoader::loadSDFont(const std::string &fontJsonPath, const std::string &fontPngPath, FontData &fontData)
{
    std::ifstream ifs(fontJsonPath);
    if (!ifs.is_open())
    {
        std::cerr << "Failed to open font JSON file: " << fontJsonPath << std::endl;
        return false;
    }
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    try
    {
        nlohmann::json j = nlohmann::json::parse(buffer.str());
        fontData.lineHeight = j["common"]["lineHeight"].get<int>();
        fontData.textureWidth = j["common"]["scaleW"].get<int>();
        fontData.textureHeight = j["common"]["scaleH"].get<int>();
        fontData.baseFontSize = j["info"]["size"].get<int>();
        for (const auto &char_json : j["chars"])
        {
            CharInfo ci;
            ci.id = char_json["id"].get<int>();
            ci.x = char_json["x"].get<int>();
            ci.y = char_json["y"].get<int>();
            ci.width = char_json["width"].get<int>();
            ci.height = char_json["height"].get<int>();
            ci.xoffset = char_json["xoffset"].get<int>();
            ci.yoffset = char_json["yoffset"].get<int>();
            ci.xadvance = char_json["xadvance"].get<int>();
            fontData.chars[ci.id] = ci;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Failed to parse font JSON: " << e.what() << std::endl;
        return false;
    }
    fontData.textureID = loadTexture(fontPngPath, fontData.textureWidth, fontData.textureHeight);
    if (fontData.textureID == 0)
    {
        std::cerr << "Failed to load font texture." << std::endl;
        return false;
    }
    fontData.isLoaded = true;
    return true;
}
