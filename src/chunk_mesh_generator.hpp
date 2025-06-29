#ifndef CHUNK_MESH_GENERATOR_HPP
#define CHUNK_MESH_GENERATOR_HPP

#include <vector>
#include <cstdint>
#include <array> // std::array を使用するため追加
#include "chunk/chunk.hpp" // chunk.hppへのパスは実際のプロジェクト構造に合わせてください
#include <glm/glm.hpp> // glm::ivec3 のために必要

struct Vertex
{
    float x, y, z;
    float r, g, b;
};

struct ChunkMeshData
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

class ChunkMeshGenerator
{
public:
    // チャンクの境界外のボクセルをチェックするために、隣接チャンクの情報も受け取るように変更
    // adjacentChunks は、Z-, Z+, X-, X+, Y-, Y+ の順に対応するチャンクへのポインタ
    // 該当する方向の隣接チャンクが存在しない場合はnullptrを渡す
    static ChunkMeshData generateMesh(const Chunk &currentChunk, 
                                      const std::array<const Chunk*, 6>& adjacentChunks);
};

#endif // CHUNK_MESH_GENERATOR_HPP