#include "chunk_manager.hpp"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <limits>
#include "chunk_mesh_generator.hpp"
#include "chunk_renderer.hpp"

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
    // 待機中の非同期タスクがあればキャンセルまたは待機 (今回は簡単のため省略)
    m_chunkRenderData.clear();
}

// プレイヤーの位置に基づいてチャンクを更新（ロード/アンロード/メッシュ更新）
void ChunkManager::update(const glm::vec3 &playerPosition)
{
    glm::ivec3 currentChunkCoord = getChunkCoordFromWorldPos(playerPosition);

    if (currentChunkCoord != m_lastPlayerChunkCoord)
    {
        loadChunksInArea(currentChunkCoord); // ここで新しいチャンクの生成も非同期で行われるように変更
        unloadDistantChunks(currentChunkCoord);
        m_lastPlayerChunkCoord = currentChunkCoord;
    }

    // 完了したチャンク生成タスクの結果を処理
    auto it_chunk_gen = m_pendingChunkGenerations.begin();
    while (it_chunk_gen != m_pendingChunkGenerations.end())
    {
        // future が ready であるかをチェック (is_ready() は C++20 以降)
        // C++11/14/17 の場合: future.wait_for(std::chrono::seconds(0)) == std::future_status::ready
        if (it_chunk_gen->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            glm::ivec3 chunkCoord = it_chunk_gen->first;
            std::shared_ptr<Chunk> newChunk = it_chunk_gen->second.get(); // 結果を取得

            if (newChunk)
            {
                m_chunks[chunkCoord] = newChunk;
                newChunk->setDirty(true); // 新しいチャンクが生成されたらメッシュ生成が必要
                
                // 新しく生成されたチャンクの隣接チャンクをダーティにする
                for (int i = 0; i < 6; ++i)
                {
                    glm::ivec3 offset = neighborOffsets[i];
                    glm::ivec3 neighborCoord = chunkCoord + offset;
                    std::shared_ptr<Chunk> neighborChunk = getChunk(neighborCoord);
                    if (neighborChunk)
                    {
                        neighborChunk->setDirty(true);
                    }
                }
            }
            it_chunk_gen = m_pendingChunkGenerations.erase(it_chunk_gen); // 完了したタスクを削除
        }
        else
        {
            ++it_chunk_gen;
        }
    }

    // ダーティなチャンクのメッシュ生成を非同期で開始
    std::vector<glm::ivec3> chunksToProcessMesh;
    for (auto &pair : m_chunks)
    {
        // 既にメッシュ生成タスクがペンディング中でない、かつダーティなチャンク
        if (pair.second->isDirty() && m_pendingMeshGenerations.find(pair.first) == m_pendingMeshGenerations.end())
        {
            chunksToProcessMesh.push_back(pair.first);
            pair.second->setDirty(false); // メッシュ生成タスクを開始したらダーティフラグをクリア
        }
    }

    for (const auto& chunkCoord : chunksToProcessMesh)
    {
        std::shared_ptr<Chunk> chunk = m_chunks[chunkCoord];
        // generateMeshForChunk を非同期で実行し、結果をマップに保存
        m_pendingMeshGenerations[chunkCoord] = std::async(std::launch::async,
                                                         &ChunkManager::generateMeshForChunk, this, chunkCoord, chunk);
    }

    // 完了したメッシュ生成タスクの結果を処理 (OpenGLリソース更新はメインスレッドで行う)
    auto it_mesh_gen = m_pendingMeshGenerations.begin();
    while (it_mesh_gen != m_pendingMeshGenerations.end())
    {
        if (it_mesh_gen->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            glm::ivec3 chunkCoord = it_mesh_gen->first;
            ChunkMeshData meshData = it_mesh_gen->second.get(); // 結果を取得

            updateChunkRenderData(chunkCoord, meshData); // OpenGLリソースの更新
            it_mesh_gen = m_pendingMeshGenerations.erase(it_mesh_gen); // 完了したタスクを削除
        }
        else
        {
            ++it_mesh_gen;
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

// ワールド座標からチャンク座標を計算するヘルパー関数
glm::ivec3 ChunkManager::getChunkCoordFromWorldPos(const glm::vec3 &worldPos) const
{
    return glm::ivec3(std::floor(worldPos.x / m_chunkSize),
                      std::floor(worldPos.y / m_chunkSize),
                      std::floor(worldPos.z / m_chunkSize));
}

// プレイヤーを中心としたエリア内のチャンクをロード（存在しない場合は生成）
void ChunkManager::loadChunksInArea(const glm::ivec3 &centerChunkCoord)
{
    for (int y = -m_renderDistance; y <= m_renderDistance; ++y)
    {
        for (int x = -m_renderDistance; x <= m_renderDistance; ++x)
        {
            for (int z = -m_renderDistance; z <= m_renderDistance; ++z)
            {
                glm::ivec3 chunkCoord = centerChunkCoord + glm::ivec3(x, y, z);
                // チャンクが存在しない、かつチャンク生成タスクがペンディング中でない場合
                if (!hasChunk(chunkCoord) && m_pendingChunkGenerations.find(chunkCoord) == m_pendingChunkGenerations.end())
                {
                    // generateChunkData を非同期で実行し、結果をマップに保存
                    m_pendingChunkGenerations[chunkCoord] = std::async(std::launch::async,
                                                                       &ChunkManager::generateChunkData, this, chunkCoord);
                }
            }
        }
    }
}

// 描画距離外に出たチャンクをアンロード
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

// チャンクのボクセルデータを生成する (非同期で実行される計算処理)
std::shared_ptr<Chunk> ChunkManager::generateChunkData(const glm::ivec3 &chunkCoord)
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
    return newChunk;
}

// チャンクのメッシュデータを生成する (非同期で実行される計算処理)
// OpenGLリソースは扱わない
ChunkMeshData ChunkManager::generateMeshForChunk(const glm::ivec3 &chunkCoord, std::shared_ptr<Chunk> chunk)
{
    if (!chunk)
    {
        std::cerr << "Error: Attempted to generate mesh data for a null chunk at "
                  << chunkCoord.x << ", " << chunkCoord.y << ", " << chunkCoord.z << std::endl;
        return ChunkMeshData(); // 空のメッシュデータを返す
    }

    // 隣接チャンクを取得 (メッシュ生成時に必要)
    // この時点では、隣接チャンクがまだロードされていない可能性があることに注意
    // または、既にロードされているが、まだメッシュが生成されていない可能性がある
    // 非同期処理を考慮し、generateMeshForChunk 内での getChunk 呼び出しは注意が必要
    // 理想的には、generateMeshForChunk に必要な隣接チャンクのボクセルデータを
    // 引数として渡すか、スナップショットを作成して渡すのがより堅牢
    // 今回はシンプルにするため、既存の getChunk を使用
    const Chunk *neighbor_neg_x = getChunk(glm::ivec3(chunkCoord.x - 1, chunkCoord.y, chunkCoord.z)).get();
    const Chunk *neighbor_pos_x = getChunk(glm::ivec3(chunkCoord.x + 1, chunkCoord.y, chunkCoord.z)).get();
    const Chunk *neighbor_neg_y = getChunk(glm::ivec3(chunkCoord.x, chunkCoord.y - 1, chunkCoord.z)).get();
    const Chunk *neighbor_pos_y = getChunk(glm::ivec3(chunkCoord.x, chunkCoord.y + 1, chunkCoord.z)).get();
    const Chunk *neighbor_neg_z = getChunk(glm::ivec3(chunkCoord.x, chunkCoord.y, chunkCoord.z - 1)).get();
    const Chunk *neighbor_pos_z = getChunk(glm::ivec3(chunkCoord.x, chunkCoord.y, chunkCoord.z + 1)).get();

    // ChunkMeshGenerator を使用してメッシュデータを生成
    ChunkMeshData meshData = ChunkMeshGenerator::generateMesh(*chunk,
                                                              neighbor_neg_x, neighbor_pos_x,
                                                              neighbor_neg_y, neighbor_pos_y,
                                                              neighbor_neg_z, neighbor_pos_z);
    return meshData;
}

// OpenGLリソースの更新はメインスレッドで行う
void ChunkManager::updateChunkRenderData(const glm::ivec3 &chunkCoord, const ChunkMeshData &meshData)
{
    auto it = m_chunkRenderData.find(chunkCoord);
    if (it != m_chunkRenderData.end())
    {
        // 既存のVAO, VBO, EBOを削除
        if (it->second.VAO != 0) glDeleteVertexArrays(1, &it->second.VAO);
        if (it->second.VBO != 0) glDeleteBuffers(1, &it->second.VBO);
        if (it->second.EBO != 0) glDeleteBuffers(1, &it->second.EBO);
        m_chunkRenderData.erase(it);
    }

    // 新しいレンダリングデータを生成
    if (!meshData.vertices.empty() && !meshData.indices.empty())
    {
        ChunkRenderData renderData = ChunkRenderer::createChunkRenderData(meshData);
        m_chunkRenderData[chunkCoord] = std::move(renderData);
    }
    else
    {
        std::cout << "Generated empty mesh data for chunk "
                  << chunkCoord.x << ", " << chunkCoord.y << ", " << chunkCoord.z << std::endl;
    }
}