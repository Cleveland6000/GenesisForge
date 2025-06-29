#ifndef CHUNK_RENDER_DATA_HPP
#define CHUNK_RENDER_DATA_HPP

#include <glad/glad.h> // GLuint のために必要

// チャンクのレンダリングに必要なデータを格納する構造体
// OpenGLリソースの管理はChunkManagerとChunkRendererに任せ、
// この構造体のデストラクタではリソースを解放しない。
struct ChunkRenderData {
    GLuint VAO = 0;   // 名前を VAO に変更
    GLuint VBO = 0;   // 名前を VBO に変更
    GLuint EBO = 0;   // 名前を EBO に変更
    GLsizei indexCount = 0; // glDrawElements のために GLsizei を使用

    ChunkRenderData() = default;

    // コピーは禁止（OpenGLリソースは共有できないため）
    ChunkRenderData(const ChunkRenderData&) = delete;
    ChunkRenderData& operator=(const ChunkRenderData&) = delete;

    // ムーブセマンティクスは許可
    // ムーブコンストラクタ
    ChunkRenderData(ChunkRenderData&& other) noexcept
        : VAO(other.VAO), VBO(other.VBO), EBO(other.EBO), indexCount(other.indexCount) {
        // ムーブ元のオブジェクトのIDを0に設定し、デストラクタが誤ってリソースを解放しないようにする
        other.VAO = 0;
        other.VBO = 0;
        other.EBO = 0;
        other.indexCount = 0;
    }

    // ムーブ代入演算子
    ChunkRenderData& operator=(ChunkRenderData&& other) noexcept {
        if (this != &other) {
            // 現在のオブジェクトが保持しているOpenGLリソースを解放
            // これはChunkManagerがdeleteChunkRenderDataを呼び出す責任があるため、ここでは何もしない
            // もしこの構造体自体がリソースの所有権を持つなら、ここで解放処理が必要になる
            // 例: if (VAO != 0) glDeleteVertexArrays(1, &VAO); など

            VAO = other.VAO;
            VBO = other.VBO;
            EBO = other.EBO;
            indexCount = other.indexCount;

            // ムーブ元のオブジェクトのIDを0に設定
            other.VAO = 0;
            other.VBO = 0;
            other.EBO = 0;
            other.indexCount = 0;
        }
        return *this;
    }
};

#endif // CHUNK_RENDER_DATA_HPP