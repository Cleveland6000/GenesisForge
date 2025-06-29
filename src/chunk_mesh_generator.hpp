#ifndef CHUNK_MESH_GENERATOR_HPP
#define CHUNK_MESH_GENERATOR_HPP

#include <vector>
#include <cstdint>
#include <array>   // std::array を使用するため追加
#include <glm/glm.hpp> // glm::ivec3 を使用するため追加

#include "chunk/chunk.hpp" // Chunkクラスの定義が必要

struct Vertex
{
    float x, y, z;
    float r, g, b; // 現在の色情報
    float u, v;    // テクスチャ座標
    float nx, ny, nz; // 法線ベクトル
};

struct ChunkMeshData
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

// 隣接チャンクのオフセットをグローバルに定義
const std::array<glm::ivec3, 6> neighborOffsets = {
    glm::ivec3(0, 0, -1), // Back face (Z-)
    glm::ivec3(0, 0, 1),  // Front face (Z+)
    glm::ivec3(-1, 0, 0), // Left face (X-)
    glm::ivec3(1, 0, 0),  // Right face (X+)
    glm::ivec3(0, -1, 0), // Bottom face (Y-)
    glm::ivec3(0, 1, 0)   // Top face (Y+)
};

class ChunkMeshGenerator
{
public:
    static ChunkMeshData generateMesh(const Chunk &chunk,
                                      const Chunk* neighbor_neg_x = nullptr, // X-方向のチャンク
                                      const Chunk* neighbor_pos_x = nullptr, // X+方向のチャンク
                                      const Chunk* neighbor_neg_y = nullptr, // Y-方向のチャンク
                                      const Chunk* neighbor_pos_y = nullptr, // Y+方向のチャンク
                                      const Chunk* neighbor_neg_z = nullptr, // Z-方向のチャンク
                                      const Chunk* neighbor_pos_z = nullptr  // Z+方向のチャンク
                                      );
};

#endif // CHUNK_MESH_GENERATOR_HPP