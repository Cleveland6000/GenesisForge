#include "FontLoader.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

// nlohmann/jsonをインクルード (あなたのdependenciesディレクトリに配置したjson.hppのパスに合わせて調整してください)
#include <json.hpp> // 例: プロジェクトルートのdependenciesフォルダにある場合

// stb_imageをインクルード (画像ローダー、もしあれば)
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.hpp> // 例: プロジェクトルートのdependenciesフォルダにある場合

// コンストラクタ
FontLoader::FontLoader() {
    // コンストラクタでの特別な初期化は不要
}

// デストラクタ
FontLoader::~FontLoader() {
    // デストラクタでのリソース解放は、FontData構造体を管理する側で行う
}

// SDFフォントアトラスとJSONメタデータをロードする関数
bool FontLoader::loadSDFont(const std::string& fontJsonPath, const std::string& fontPngPath, FontData& fontData) {
    fontData.isLoaded = false;

    // --- 1. JSONメタデータのロードとパース ---
    std::ifstream jsonFile(fontJsonPath);
    if (!jsonFile.is_open()) {
        std::cerr << "Error: Could not open font JSON file: " << fontJsonPath << std::endl;
        return false;
    }

    nlohmann::json jsonDoc;
    try {
        jsonFile >> jsonDoc;
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "Error: Could not parse font JSON file '" << fontJsonPath << "': " << e.what() << std::endl;
        return false;
    }

    // commonセクションからフォント情報を取得
    if (jsonDoc.count("common") && jsonDoc["common"].is_object()) {
        fontData.lineHeight = jsonDoc["common"].value("lineHeight", 0);
        fontData.textureWidth = jsonDoc["common"].value("scaleW", 0);
        fontData.textureHeight = jsonDoc["common"].value("scaleH", 0);
    } else {
        std::cerr << "Error: 'common' section not found or invalid in JSON: " << fontJsonPath << std::endl;
        return false;
    }

    // charsセクションから各文字の情報を取得
    if (jsonDoc.count("chars") && jsonDoc["chars"].is_array()) {
        for (const auto& charEntry : jsonDoc["chars"]) {
            CharInfo charInfo;
            charInfo.id = charEntry.value("id", 0);
            charInfo.x = charEntry.value("x", 0);
            charInfo.y = charEntry.value("y", 0);
            charInfo.width = charEntry.value("width", 0);
            charInfo.height = charEntry.value("height", 0);
            charInfo.xoffset = charEntry.value("xoffset", 0);
            charInfo.yoffset = charEntry.value("yoffset", 0);
            charInfo.xadvance = charEntry.value("xadvance", 0);
            fontData.chars[charInfo.id] = charInfo;
        }
    } else {
        std::cerr << "Error: 'chars' section not found or invalid in JSON: " << fontJsonPath << std::endl;
        return false;
    }
    
    // カーニング情報をロード（オプション）
    // BMFont JSONはkerningセクションを持つことがありますが、
    // msdf-bmfont-xmlはkersセクションを持たないことがあります。
    // 必要であればここに追加します。

    // --- 2. PNGアトラス画像のロードとOpenGLテクスチャの作成 ---
    fontData.textureID = loadTexture(fontPngPath, fontData.textureWidth, fontData.textureHeight);
    if (fontData.textureID == 0) {
        std::cerr << "Error: Failed to load font texture from PNG: " << fontPngPath << std::endl;
        return false;
    }

    fontData.isLoaded = true;
    std::cout << "SDF font '" << fontJsonPath << "' loaded successfully." << std::endl;
    return true;
}

// PNG画像をロードしてOpenGLテクスチャを作成するヘルパー関数
GLuint FontLoader::loadTexture(const std::string& imagePath, int& width, int& height) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // テクスチャパラメータを設定
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // ミップマップを使用
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int nrChannels;
    // stbi_set_flip_vertically_on_load(true); // OpenGLはY軸が下向きなので画像を反転する必要がある場合がある
                                           // MSDFの場合、Y座標は通常上から下に増えるので反転しないことが多い
    unsigned char* data = stbi_load(imagePath.c_str(), &width, &height, &nrChannels, 0);

    if (data) {
        GLenum format;
        if (nrChannels == 1) format = GL_RED;
        else if (nrChannels == 3) format = GL_RGB;
        else if (nrChannels == 4) format = GL_RGBA;
        else {
            std::cerr << "Error: Unsupported number of channels for texture: " << nrChannels << std::endl;
            stbi_image_free(data);
            return 0;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D); // ミップマップ生成

        stbi_image_free(data);
        std::cout << "Texture '" << imagePath << "' loaded. Width: " << width << ", Height: " << height << ", Channels: " << nrChannels << std::endl;
    } else {
        std::cerr << "Error: Failed to load image: " << imagePath << " - " << stbi_failure_reason() << std::endl;
        glDeleteTextures(1, &textureID);
        return 0;
    }

    return textureID;
}