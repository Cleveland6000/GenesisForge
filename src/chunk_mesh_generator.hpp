#ifndef CHUNK_MESH_GENERATOR_HPP
#define CHUNK_MESH_GENERATOR_HPP

#include <vector>
#include <cstdint>
#include <array>
#include <glm/glm.hpp>

#include "chunk/chunk.hpp"

struct Vertex
{
    float x, y, z;
    float r, g, b;
    float u, v;
    float nx, ny, nz;
    float ao; // <--- AO値を格納する新しい属性を追加
};

struct ChunkMeshData
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

// 隣接チャンクのオフセットをグローバルに定義 (変更なし)
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
                                      const Chunk* neighbor_neg_x = nullptr,
                                      const Chunk* neighbor_pos_x = nullptr,
                                      const Chunk* neighbor_neg_y = nullptr,
                                      const Chunk* neighbor_pos_y = nullptr,
                                      const Chunk* neighbor_neg_z = nullptr,
                                      const Chunk* neighbor_pos_z = nullptr
                                      );
};

#endif // CHUNK_MESH_GENERATOR_HPP