#include "chunk_processor.hpp"
#include <iostream>

ChunkProcessor::ChunkProcessor(int chunkSize, std::unique_ptr<TerrainGenerator> terrainGenerator)
    : m_chunkSize(chunkSize), m_terrainGenerator(std::move(terrainGenerator))
{
    // TerrainGenerator は ChunkManager から move されるため、ここでは何もしない
}

// チャンクのボクセルデータを生成する (非同期で実行される計算処理)
std::shared_ptr<Chunk> ChunkProcessor::generateChunkData(const glm::ivec3& chunkCoord)
{
    std::shared_ptr<Chunk> newChunk = std::make_shared<Chunk>(m_chunkSize, chunkCoord);
    if (!m_terrainGenerator)
    {
        std::cerr << "Error: TerrainGenerator instance is not initialized in ChunkProcessor.\n";
        return newChunk;
    }

    std::vector<int> heightMap(m_chunkSize * m_chunkSize);
    for (int x = 0; x < m_chunkSize; ++x)
    {
        for (int z = 0; z < m_chunkSize; ++z)
        {
            float worldX = (float)x + (float)chunkCoord.x * m_chunkSize;
            float worldZ = (float)z + (float)chunkCoord.z * m_chunkSize;
            heightMap[x + z * m_chunkSize] = m_terrainGenerator->getTerrainHeight(worldX, worldZ);
        }
    }

    std::vector<bool> tempVoxels(m_chunkSize * m_chunkSize * m_chunkSize);

    for (int z = 0; z < m_chunkSize; ++z)
    {
        for (int y = 0; y < m_chunkSize; ++y)
        {
            for (int x = 0; x < m_chunkSize; ++x)
            {
                float worldY = (float)y + (float)chunkCoord.y * m_chunkSize;

                bool isSolid;
                if (worldY < m_terrainGenerator->getGroundLevel())
                {
                    isSolid = true;
                }
                else
                {
                    int terrainHeightAtXZ = heightMap[x + z * m_chunkSize];
                    isSolid = (worldY < terrainHeightAtXZ);
                }

                size_t index = static_cast<size_t>(x + y * m_chunkSize + z * m_chunkSize * m_chunkSize);
                tempVoxels[index] = isSolid;
            }
        }
    }
    newChunk->setVoxels(tempVoxels);
    return newChunk;
}

// 隣接チャンク取得のためのヘルパー関数
const Chunk* ChunkProcessor::getNeighbor(const glm::ivec3& currentChunkCoord, const glm::ivec3& offset,
                                         NeighborChunkProvider* neighborProvider)
{
    if (!neighborProvider) return nullptr; // プロバイダがない場合はnullptrを返す
    std::shared_ptr<Chunk> neighborChunk = neighborProvider->getChunk(currentChunkCoord + offset);
    return neighborChunk.get(); // shared_ptr から生ポインタを取得
}

// チャンクのメッシュデータを生成する (非同期で実行される計算処理)
ChunkMeshData ChunkProcessor::generateMeshForChunk(const glm::ivec3& chunkCoord, std::shared_ptr<Chunk> chunk,
                                                   NeighborChunkProvider* neighborProvider)
{
    if (!chunk)
    {
        std::cerr << "Error: Attempted to generate mesh data for a null chunk at "
                  << chunkCoord.x << ", " << chunkCoord.y << ", " << chunkCoord.z << std::endl;
        return ChunkMeshData(); // 空のメッシュデータを返す
    }

    // 隣接チャンクのデータを取得
    const Chunk *neighbor_neg_x = getNeighbor(chunkCoord, glm::ivec3(-1, 0, 0), neighborProvider);
    const Chunk *neighbor_pos_x = getNeighbor(chunkCoord, glm::ivec3(1, 0, 0), neighborProvider);
    const Chunk *neighbor_neg_y = getNeighbor(chunkCoord, glm::ivec3(0, -1, 0), neighborProvider);
    const Chunk *neighbor_pos_y = getNeighbor(chunkCoord, glm::ivec3(0, 1, 0), neighborProvider);
    const Chunk *neighbor_neg_z = getNeighbor(chunkCoord, glm::ivec3(0, 0, -1), neighborProvider);
    const Chunk *neighbor_pos_z = getNeighbor(chunkCoord, glm::ivec3(0, 0, 1), neighborProvider);

    // ChunkMeshGenerator を使用してメッシュデータを生成
    ChunkMeshData meshData = ChunkMeshGenerator::generateMesh(*chunk,
                                                              neighbor_neg_x, neighbor_pos_x,
                                                              neighbor_neg_y, neighbor_pos_y,
                                                              neighbor_neg_z, neighbor_pos_z);
    return meshData;
}