#include "chunk_mesh_generator.hpp"
#include <glm/glm.hpp>
#include <array>
#include <vector>
#include <iostream>
#include <random>

ChunkMeshData ChunkMeshGenerator::generateMesh(const Chunk &chunk,
                                               const Chunk *neighbor_neg_x,
                                               const Chunk *neighbor_pos_x,
                                               const Chunk *neighbor_neg_y,
                                               const Chunk *neighbor_pos_y,
                                               const Chunk *neighbor_neg_z,
                                               const Chunk *neighbor_pos_z)
{
    ChunkMeshData meshData;
    int chunkSize = chunk.getSize();
    const std::vector<bool> &voxels = chunk.getVoxels();

    VoxelAccessor voxelAccessor(chunk,
                                neighbor_neg_x, neighbor_pos_x,
                                neighbor_neg_y, neighbor_pos_y,
                                neighbor_neg_z, neighbor_pos_z);

    FaceBaker faceBaker(voxelAccessor, chunkSize);

    glm::ivec3 chunkCoord = chunk.getCoord();

    std::seed_seq seed_seq{
        static_cast<std::uint32_t>(chunkCoord.x),
        static_cast<std::uint32_t>(chunkCoord.y),
        static_cast<std::uint32_t>(chunkCoord.z)};
    std::mt19937 rng(seed_seq);

    std::uniform_int_distribution<int> rotation_dist(0, 3);
    std::uniform_int_distribution<int> flip_dist(0, 1);

    meshData.vertices.reserve(chunkSize * chunkSize * chunkSize * 4 * 6);
    meshData.indices.reserve(chunkSize * chunkSize * chunkSize * 6 * 6);

    for (int z = 0; z < chunkSize; ++z)
    {
        for (int y = 0; y < chunkSize; ++y)
        {
            for (int x = 0; x < chunkSize; ++x)
            {
                // 現在のボクセルがソリッドかどうかはVoxelAccessor経由で確認
                // getVoxelValue は VoxelAccessor に移動し、isSolid がその機能を含むようになりました
                if (voxelAccessor.isSolid(x, y, z)) // チャンク内のボクセルがソリッド
                {
                    int rotationAmount = rotation_dist(rng);
                    bool flipHorizontal = (flip_dist(rng) == 1);

                    for (int i = 0; i < 6; ++i)
                    {
                        glm::ivec3 offset = neighborOffsets[i];
                        int neighborX = x + offset.x;
                        int neighborY = y + offset.y;
                        int neighborZ = z + offset.z;

                        if (!voxelAccessor.isSolid(neighborX, neighborY, neighborZ))
                        {
                            faceBaker.bakeFace(meshData, x, y, z, i, rotationAmount, flipHorizontal);
                        }
                    }
                }
            }
        }
    }
    return meshData;
}