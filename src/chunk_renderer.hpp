// src/chunk_renderer.hpp
#ifndef CHUNK_RENDERER_HPP
#define CHUNK_RENDERER_HPP

#include <glad/glad.h>
#include "renderer.hpp" // ChunkRenderData の定義を含む
#include "chunk_mesh_generator.hpp" // ChunkMeshData の定義を含む

class ChunkRenderer {
public:
    // ChunkMeshData から OpenGL 用の ChunkRenderData を生成する
    // この関数はGPUリソースを作成します
    static ChunkRenderData createChunkRenderData(const ChunkMeshData& meshData);
};

#endif // CHUNK_RENDERER_HPP