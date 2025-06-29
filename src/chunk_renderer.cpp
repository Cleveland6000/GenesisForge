#include "chunk_renderer.hpp"
#include <iostream>

ChunkRenderData ChunkRenderer::createChunkRenderData(const ChunkMeshData& meshData) {
    ChunkRenderData renderData;

    if (meshData.vertices.empty() || meshData.indices.empty()) {
        return renderData;
    }

    glGenVertexArrays(1, &renderData.VAO);
    glGenBuffers(1, &renderData.VBO);
    glGenBuffers(1, &renderData.EBO);

    glBindVertexArray(renderData.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, renderData.VBO);
    glBufferData(GL_ARRAY_BUFFER, meshData.vertices.size() * sizeof(Vertex), meshData.vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderData.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshData.indices.size() * sizeof(unsigned int), meshData.indices.data(), GL_STATIC_DRAW);

    // 頂点属性ポインタを設定
    // 位置属性 (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, x));
    glEnableVertexAttribArray(0);
    // 色属性 (location = 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, r));
    glEnableVertexAttribArray(1);
    // テクスチャ座標属性 (location = 2) を追加
    // Vertex構造体内の 'u' メンバーへのオフセットを指定
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, u));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    renderData.indexCount = static_cast<GLsizei>(meshData.indices.size());

    return renderData;
}