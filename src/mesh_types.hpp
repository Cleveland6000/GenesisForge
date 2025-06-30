#ifndef MESH_TYPES_HPP
#define MESH_TYPES_HPP

#include <vector>
#include <glm/glm.hpp> // 必要に応じて

// Vertex 構造体の定義
struct Vertex
{
    float x, y, z;
    float r, g, b;
    float u, v;
    float nx, ny, nz;
    float ao; // AO値を格納する新しい属性
};

// ChunkMeshData 構造体の定義
struct ChunkMeshData
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

#endif // MESH_TYPES_HPP
