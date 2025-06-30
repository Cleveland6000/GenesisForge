#ifndef CHUNK_MESH_GENERATOR_HPP
#define CHUNK_MESH_GENERATOR_HPP

#include <vector>
#include <cstdint>
#include <array>
#include <glm/glm.hpp>

#include "chunk/chunk.hpp" // Chunkクラスの定義のため
#include "mesh_types.hpp"   // 新しく追加
#include "voxel_accessor.hpp"
#include "face_baker.hpp"

// Vertex と ChunkMeshData の定義はmesh_types.hppに移動したので、ここからは削除
// struct Vertex {...};
// struct ChunkMeshData {...};

// 隣接チャンクのオフセットはChunkMeshGeneratorが直接使用するため、ここに維持
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