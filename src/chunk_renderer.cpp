#include "chunk_renderer.hpp"
#include "chunk_mesh_generator.hpp" // ChunkMeshData の完全な定義のために必要
#include <glad/glad.h> // OpenGL の関数を呼び出すために必要
#include <iostream>

// ... (既存の createChunkRenderData の実装) ...

ChunkRenderData ChunkRenderer::createChunkRenderData(const ChunkMeshData& meshData) {
    ChunkRenderData data;

    glGenVertexArrays(1, &data.VAO);   // vaoID を VAO に変更
    glGenBuffers(1, &data.VBO);     // vboID を VBO に変更
    glGenBuffers(1, &data.EBO);     // eboID を EBO に変更

    glBindVertexArray(data.VAO);   // vaoID を VAO に変更

    glBindBuffer(GL_ARRAY_BUFFER, data.VBO); // vboID を VBO に変更
    glBufferData(GL_ARRAY_BUFFER, meshData.vertices.size() * sizeof(float), meshData.vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.EBO); // eboID を EBO に変更
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshData.indices.size() * sizeof(unsigned int), meshData.indices.data(), GL_STATIC_DRAW);

    // 頂点属性ポインタを設定
    // 位置 (3 floats)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // 法線 (3 floats)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // テクスチャ座標 (2 floats)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0); // VAOのバインドを解除

    data.indexCount = meshData.indices.size();

    return data;
}

// OpenGLリソースを解放する実装
void ChunkRenderer::deleteChunkRenderData(const ChunkRenderData& data) {
    if (data.VAO != 0) { // VAO を VAO に変更
        glDeleteVertexArrays(1, &data.VAO); // vaoID を VAO に変更
    }
    if (data.VBO != 0) { // VBO を VBO に変更
        glDeleteBuffers(1, &data.VBO);   // vboID を VBO に変更
    }
    if (data.EBO != 0) { // EBO を EBO に変更
        glDeleteBuffers(1, &data.EBO);   // eboID を EBO に変更
    }
}

// ... (既存の renderChunk の実装) ...