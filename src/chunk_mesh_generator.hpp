#ifndef CHUNK_MESH_GENERATOR_HPP
#define CHUNK_MESH_GENERATOR_HPP

#include <vector>
#include <cstdint>
#include "chunk/chunk.hpp"

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
    static ChunkMeshData generateMesh(const Chunk &chunk);
};

#endif // CHUNK_MESH_GENERATOR_HPP
