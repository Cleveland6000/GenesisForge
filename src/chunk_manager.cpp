#include "chunk_manager.hpp"
#include <iostream>
#include <chrono>
#include "chunk_mesh_generator.hpp"

// コンストラクタ: TerrainGenerator のコンストラクタに新しい引数を渡す
ChunkManager::ChunkManager(int chunkSize, int renderDistance, unsigned int noiseSeed, float noiseScale, 
                           int worldMaxHeight, int groundLevel, int octaves, float lacunarity, float persistence)
    : m_chunkSize(chunkSize), m_renderDistance(renderDistance),
      m_terrainGenerator(std::make_unique<TerrainGenerator>(noiseSeed, noiseScale, worldMaxHeight, groundLevel,
                                                          octaves, lacunarity, persistence)) // TerrainGenerator をここで初期化
{
    std::cout << "ChunkManager constructor called. ChunkSize: " << m_chunkSize 
              << ", RenderDistance: " << m_renderDistance << std::endl;
}

// デストラクタ
ChunkManager::~ChunkManager() {
    std::cout << "ChunkManager destructor called." << std::endl;
    m_chunkRenderData.clear();
}

// プレイヤーの位置に基づいてチャンクを更新
void ChunkManager::update(const glm::vec3& playerPosition) {
    glm::ivec3 centerChunkCoord = getChunkCoordFromWorldPos(playerPosition);

    loadChunksInArea(centerChunkCoord);
    unloadDistantChunks(centerChunkCoord);

    for (auto& pair : m_chunks) {
        if (pair.second->isDirty()) {
            updateChunkMesh(pair.first, pair.second);
            pair.second->setDirty(false);
        }
    }
}

// 指定されたワールド座標のチャンクが存在するかどうかをチェック
bool ChunkManager::hasChunk(const glm::ivec3& chunkCoord) const {
    return m_chunks.count(chunkCoord) > 0;
}

// 指定されたチャンク座標のチャンクを取得
std::shared_ptr<Chunk> ChunkManager::getChunk(const glm::ivec3& chunkCoord) {
    auto it = m_chunks.find(chunkCoord);
    if (it != m_chunks.end()) {
        return it->second;
    }
    return nullptr;
}

// チャンクを生成して初期化
std::shared_ptr<Chunk> ChunkManager::generateChunk(const glm::ivec3& chunkCoord) {
    std::cout << "Generating chunk at: (" << chunkCoord.x << ", " << chunkCoord.y << ", " << chunkCoord.z << ")\n";
    std::shared_ptr<Chunk> newChunk = std::make_shared<Chunk>(m_chunkSize);
    
    if (!m_terrainGenerator) {
        std::cerr << "Error: TerrainGenerator instance is not initialized in ChunkManager.\n";
        return newChunk;
    }

    for (int x = 0; x < m_chunkSize; ++x) {
        for (int y = 0; y < m_chunkSize; ++y) {
            for (int z = 0; z < m_chunkSize; ++z) {
                // グローバルワールド座標を計算
                float worldX = (float)x + (float)chunkCoord.x * m_chunkSize;
                float worldY = (float)y + (float)chunkCoord.y * m_chunkSize; // グローバルY座標も考慮
                float worldZ = (float)z + (float)chunkCoord.z * m_chunkSize;

                // TerrainGenerator を使用してボクセルがソリッドであるか判定
                bool isSolid = m_terrainGenerator->isVoxelSolid(worldX, worldY, worldZ);
                
                newChunk->setVoxel(x, y, z, isSolid);
            }
        }
    }
    newChunk->setDirty(true);
    return newChunk;
}

// チャンクのメッシュを生成し、レンダリングデータを更新
void ChunkManager::updateChunkMesh(const glm::ivec3& chunkCoord, std::shared_ptr<Chunk> chunk) {
    ChunkMeshData meshData = ChunkMeshGenerator::generateMesh(*chunk, 1.0f); // 1.0f は cubeSpacing
    m_chunkRenderData[chunkCoord] = ChunkRenderer::createChunkRenderData(meshData);
    std::cout << "Updated mesh for chunk at: (" << chunkCoord.x << ", " << chunkCoord.y << ", " << chunkCoord.z << ")\n";
}

// 指定された座標範囲内のチャンクをロード
void ChunkManager::loadChunksInArea(const glm::ivec3& centerChunkCoord) {
    for (int x = -m_renderDistance; x <= m_renderDistance; ++x) {
        for (int z = -m_renderDistance; z <= m_renderDistance; ++z) {
            glm::ivec3 currentChunkCoord = glm::ivec3(centerChunkCoord.x + x, 0, centerChunkCoord.z + z); // Y座標は仮に0とする

            if (!hasChunk(currentChunkCoord)) {
                std::shared_ptr<Chunk> newChunk = generateChunk(currentChunkCoord);
                m_chunks[currentChunkCoord] = newChunk;
            }
        }
    }
}

// 不要なチャンクをアンロード
void ChunkManager::unloadDistantChunks(const glm::ivec3& centerChunkCoord) {
    std::vector<glm::ivec3> chunksToUnload;
    for (auto const& [coord, chunk] : m_chunks) {
        int dist_x = std::abs(coord.x - centerChunkCoord.x);
        int dist_z = std::abs(coord.z - centerChunkCoord.z);

        if (dist_x > m_renderDistance || dist_z > m_renderDistance) {
            chunksToUnload.push_back(coord);
        }
    }

    for (const auto& coord : chunksToUnload) {
        std::cout << "Unloading chunk at: (" << coord.x << ", " << coord.y << ", " << coord.z << ")\n";
        m_chunkRenderData.erase(coord);
        m_chunks.erase(coord);
    }
}

// ワールド座標からチャンク座標を計算
glm::ivec3 ChunkManager::getChunkCoordFromWorldPos(const glm::vec3& worldPos) const {
    int chunkX = static_cast<int>(std::floor(worldPos.x / m_chunkSize));
    int chunkZ = static_cast<int>(std::floor(worldPos.z / m_chunkSize));
    return glm::ivec3(chunkX, 0, chunkZ);
}
