#include "FontLoader.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

// nlohmann/jsonをインクルード (あなたのdependenciesディレクトリに配置したjson.hppのパスに合わせて調整してください)
#include <json.hpp> // 例: プロジェクトルートのdependenciesフォルダにある場合

// stb_imageをインクルード (画像ローダー、もしあれば)
// STB_IMAGE_IMPLEMENTATION はこのファイルで一度だけ定義し、stb_image.h の前に置きます。
// 通常はstb_image.hであり、stb_image.hppではないことに注意してください。
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.hpp> // Corrected: assumed to be stb_image.h

FontLoader::FontLoader() {}
FontLoader::~FontLoader() {}

// PNG画像をロードしてOpenGLテクスチャを作成するヘルパー関数
GLuint FontLoader::loadTexture(const std::string& imagePath, int& width, int& height) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // テクスチャパラメータを設定
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // SDFはミップマップが重要
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int nrChannels;
    unsigned char *data = stbi_load(imagePath.c_str(), &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = GL_RGBA; // MSDFアトラスは通常RGBA (R, G, B, A または アルファのみ)
        if (nrChannels == 3) format = GL_RGB;
        else if (nrChannels == 1) format = GL_RED; // たとえREDでも、SDFシェーダーはRGBAを期待する場合があります

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cerr << "Failed to load texture: " << imagePath << std::endl;
        stbi_image_free(data);
        return 0;
    }
    stbi_image_free(data);
    return textureID;
}


bool FontLoader::loadSDFont(const std::string& fontJsonPath, const std::string& fontPngPath, FontData& fontData) {
    std::ifstream ifs(fontJsonPath);
    if (!ifs.is_open()) {
        std::cerr << "Failed to open font JSON file: " << fontJsonPath << std::endl;
        return false;
    }

    std::stringstream buffer;
    buffer << ifs.rdbuf();
    std::string jsonStr = buffer.str();

    try {
        // nlohmann::json を使ってJSON文字列をパース
        nlohmann::json j = nlohmann::json::parse(jsonStr);

        // JSONデータからFontData構造体に値を割り当てる
        fontData.lineHeight = j["common"]["lineHeight"].get<int>();
        fontData.textureWidth = j["common"]["scaleW"].get<int>();
        fontData.textureHeight = j["common"]["scaleH"].get<int>();
        fontData.baseFontSize = j["info"]["size"].get<int>(); // ★ここを有効化します★

        // 各文字の情報をパース
        for (const auto& char_json : j["chars"]) {
            CharInfo charInfo;
            charInfo.id = char_json["id"].get<int>();
            charInfo.x = char_json["x"].get<int>();
            charInfo.y = char_json["y"].get<int>();
            charInfo.width = char_json["width"].get<int>();
            charInfo.height = char_json["height"].get<int>();
            charInfo.xoffset = char_json["xoffset"].get<int>();
            charInfo.yoffset = char_json["yoffset"].get<int>();
            charInfo.xadvance = char_json["xadvance"].get<int>();
            fontData.chars[charInfo.id] = charInfo;
        }

    } catch (const nlohmann::json::exception& e) { // nlohmann::json の例外を捕捉
        std::cerr << "Failed to parse font JSON (nlohmann::json error): " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) { // その他の標準例外を捕捉
        std::cerr << "Failed to parse font JSON or data missing (std::exception): " << e.what() << std::endl;
        return false;
    }

    fontData.textureID = loadTexture(fontPngPath, fontData.textureWidth, fontData.textureHeight);
    if (fontData.textureID == 0) {
        std::cerr << "Failed to load font texture." << std::endl;
        return false;
    }

    fontData.isLoaded = true;
    return true;
}
