// src/chunk_renderer.cpp
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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    // 色属性 (location = 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // テクスチャ座標属性 (location = 2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    // 法線属性 (location = 3)
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);
    // AO属性 (location = 4) <--- 新しく追加
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(11 * sizeof(float)));
    glEnableVertexAttribArray(4);

    glBindVertexArray(0); // VAOのバインドを解除
    glBindBuffer(GL_ARRAY_BUFFER, 0); // VBOのバインドを解除
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // EBOのバインドを解除

    renderData.indexCount = static_cast<GLsizei>(meshData.indices.size());
    return renderData;
}