#include "chunk_manager.hpp"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <limits>
#include "chunk_mesh_generator.hpp" // neighborOffsets が定義されるため、このインクルードで解決されます

// コンストラクタ
ChunkManager::ChunkManager(int chunkSize, int renderDistanceXZ, unsigned int noiseSeed, float noiseScale,
                           int worldMaxHeight, int groundLevel, int octaves, float lacunarity, float persistence)
    : m_chunkSize(chunkSize), m_renderDistance(renderDistanceXZ),
      m_terrainGenerator(std::make_unique<TerrainGenerator>(noiseSeed, noiseScale, worldMaxHeight, groundLevel,
                                                            octaves, lacunarity, persistence)),
      m_lastPlayerChunkCoord(std::numeric_limits<int>::max())
{
    std::cout << "ChunkManager constructor called. ChunkSize: " << m_chunkSize
              << ", RenderDistanceXZ: " << m_renderDistance << std::endl;
}

// デストラクタ
ChunkManager::~ChunkManager()
{
    std::cout << "ChunkManager destructor called." << std::endl;
    m_chunkRenderData.clear();
}

// プレイヤーの位置に基づいてチャンクを更新（ロード/アンロード/メッシュ更新）
void ChunkManager::update(const glm::vec3 &playerPosition)
{
    glm::ivec3 currentChunkCoord = getChunkCoordFromWorldPos(playerPosition);

    if (currentChunkCoord != m_lastPlayerChunkCoord)
    {
        loadChunksInArea(currentChunkCoord);
        unloadDistantChunks(currentChunkCoord);
        m_lastPlayerChunkCoord = currentChunkCoord;

        for (auto &pair : m_chunks)
        {
            if (pair.second->isDirty())
            {
                updateChunkMesh(pair.first, pair.second);
                pair.second->setDirty(false);
            }
        }
    }
}

// 指定されたワールド座標のチャンクが存在するかどうかをチェックします
bool ChunkManager::hasChunk(const glm::ivec3 &chunkCoord) const
{
    return m_chunks.count(chunkCoord) > 0;
}

// 指定されたチャンク座標のチャンクを取得します
std::shared_ptr<Chunk> ChunkManager::getChunk(const glm::ivec3 &chunkCoord)
{
    auto it = m_chunks.find(chunkCoord);
    if (it != m_chunks.end())
    {
        return it->second;
    }
    return nullptr;
}

// チャンクを生成して初期化（ボクセルデータを一括設定）
std::shared_ptr<Chunk> ChunkManager::generateChunk(const glm::ivec3 &chunkCoord)
{
    std::shared_ptr<Chunk> newChunk = std::make_shared<Chunk>(m_chunkSize, chunkCoord);
    if (!m_terrainGenerator)
    {
        std::cerr << "Error: TerrainGenerator instance is not initialized in ChunkManager.\n";
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
    newChunk->setDirty(true);
    return newChunk;
}

// チャンクのメッシュを生成し、レンダリングデータを更新します
void ChunkManager::updateChunkMesh(const glm::ivec3 &chunkCoord, std::shared_ptr<Chunk> chunk)
{
    // 隣接チャンクを取得
    const Chunk *neighbor_neg_x = getChunk(glm::ivec3(chunkCoord.x - 1, chunkCoord.y, chunkCoord.z)).get();
    const Chunk *neighbor_pos_x = getChunk(glm::ivec3(chunkCoord.x + 1, chunkCoord.y, chunkCoord.z)).get();
    const Chunk *neighbor_neg_y = getChunk(glm::ivec3(chunkCoord.x, chunkCoord.y - 1, chunkCoord.z)).get();
    const Chunk *neighbor_pos_y = getChunk(glm::ivec3(chunkCoord.x, chunkCoord.y + 1, chunkCoord.z)).get();
    const Chunk *neighbor_neg_z = getChunk(glm::ivec3(chunkCoord.x, chunkCoord.y, chunkCoord.z - 1)).get();
    const Chunk *neighbor_pos_z = getChunk(glm::ivec3(chunkCoord.x, chunkCoord.y, chunkCoord.z + 1)).get();

    // ChunkMeshGenerator を使用してメッシュデータを生成します
    // 隣接チャンクのポインタを渡す
    ChunkMeshData meshData = ChunkMeshGenerator::generateMesh(
        *chunk,
        neighbor_neg_x, neighbor_pos_x,
        neighbor_neg_y, neighbor_pos_y,
        neighbor_neg_z, neighbor_pos_z);

    // レンダリングデータを更新します
    m_chunkRenderData[chunkCoord] = ChunkRenderer::createChunkRenderData(meshData);
}

// 指定された座標範囲内のチャンクをロードします
void ChunkManager::loadChunksInArea(const glm::ivec3 &centerChunkCoord)
{
    for (int x = -m_renderDistance; x <= m_renderDistance; ++x)
    {
        for (int y = -m_renderDistance; y <= m_renderDistance; ++y)
        {
            for (int z = -m_renderDistance; z <= m_renderDistance; ++z)
            {
                glm::ivec3 currentChunkCoord = glm::ivec3(centerChunkCoord.x + x, centerChunkCoord.y + y, centerChunkCoord.z + z);

                if (!hasChunk(currentChunkCoord))
                {
                    std::shared_ptr<Chunk> newChunk = generateChunk(currentChunkCoord);
                    m_chunks[currentChunkCoord] = newChunk;

                    // 新しいチャンクがロードされたときに、その隣接チャンク（既に存在する場合）もダーティにする
                    for (int i = 0; i < 6; ++i)
                    {
                        glm::ivec3 offset = neighborOffsets[i];
                        glm::ivec3 neighborCoord = currentChunkCoord + offset;
                        std::shared_ptr<Chunk> neighborChunk = getChunk(neighborCoord);
                        if (neighborChunk)
                        {
                            neighborChunk->setDirty(true);
                        }
                    }
                }
            }
        }
    }
}

// 不要なチャンクをアンロード（描画距離外に出たチャンク）
void ChunkManager::unloadDistantChunks(const glm::ivec3 &centerChunkCoord)
{
    std::vector<glm::ivec3> chunksToUnload;
    for (auto const &[coord, chunk] : m_chunks)
    {
        int dist_x = std::abs(coord.x - centerChunkCoord.x);
        int dist_y = std::abs(coord.y - centerChunkCoord.y);
        int dist_z = std::abs(coord.z - centerChunkCoord.z);

        if (dist_x > m_renderDistance || dist_y > m_renderDistance || dist_z > m_renderDistance)
        {
            chunksToUnload.push_back(coord);
        }
    }

    for (const auto &coord : chunksToUnload)
    {
        // チャンクがアンロードされるときに、その隣接チャンク（まだ存在する場合）もダーティにする
        for (int i = 0; i < 6; ++i)
        {
            glm::ivec3 offset = neighborOffsets[i];
            glm::ivec3 neighborCoord = coord + offset;
            std::shared_ptr<Chunk> neighborChunk = getChunk(neighborCoord);
            if (neighborChunk)
            {
                neighborChunk->setDirty(true);
            }
        }

        m_chunkRenderData.erase(coord);
        m_chunks.erase(coord);
    }
}

// ワールド座標からチャンク座標を計算します
glm::ivec3 ChunkManager::getChunkCoordFromWorldPos(const glm::vec3 &worldPos) const
{
    int chunkX = static_cast<int>(std::floor(worldPos.x / m_chunkSize));
    int chunkY = static_cast<int>(std::floor(worldPos.y / m_chunkSize));
    int chunkZ = static_cast<int>(std::floor(worldPos.z / m_chunkSize));
    return glm::ivec3(chunkX, chunkY, chunkZ);
}