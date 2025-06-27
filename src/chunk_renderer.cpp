// src/chunk_renderer.cpp
#include "chunk_renderer.hpp"
#include <iostream>

ChunkRenderData ChunkRenderer::createChunkRenderData(const ChunkMeshData& meshData) {
    ChunkRenderData renderData;

    if (meshData.vertices.empty() || meshData.indices.empty()) {
        std::cerr << "Warning: Attempted to create ChunkRenderData from empty mesh data.\n";
        return renderData; // 空のデータオブジェクトを返す
    }

    // VAO, VBO, EBO を生成
    glGenVertexArrays(1, &renderData.VAO);
    glGenBuffers(1, &renderData.VBO);
    glGenBuffers(1, &renderData.EBO);

    // VAO をバインド
    glBindVertexArray(renderData.VAO);

    // VBO に頂点データをバインドしてアップロード
    glBindBuffer(GL_ARRAY_BUFFER, renderData.VBO);
    glBufferData(GL_ARRAY_BUFFER, meshData.vertices.size() * sizeof(Vertex), meshData.vertices.data(), GL_STATIC_DRAW);

    // EBO にインデックスデータをバインドしてアップロード
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderData.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshData.indices.size() * sizeof(unsigned int), meshData.indices.data(), GL_STATIC_DRAW);

    // 頂点属性ポインタを設定
    // 位置属性 (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, x));
    glEnableVertexAttribArray(0);
    // 色属性 (location = 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, r));
    glEnableVertexAttribArray(1);

    // VAO のバインドを解除
    glBindVertexArray(0);

    // インデックス数を設定
    renderData.indexCount = static_cast<GLsizei>(meshData.indices.size());

    return renderData;
}

void ChunkRenderer::deleteChunkRenderData(ChunkRenderData& renderData) {
    // VAO, VBO, EBO が有効な場合に削除します
    if (renderData.VAO != 0) {
        glDeleteVertexArrays(1, &renderData.VAO);
        renderData.VAO = 0; // 無効化
    }
    if (renderData.VBO != 0) {
        glDeleteBuffers(1, &renderData.VBO);
        renderData.VBO = 0; // 無効化
    }
    if (renderData.EBO != 0) {
        glDeleteBuffers(1, &renderData.EBO);
        renderData.EBO = 0; // 無効化
    }
    renderData.indexCount = 0; // インデックス数もリセット
    // std::cout << "ChunkRenderData resources deleted.\n"; // デバッグ用
}

