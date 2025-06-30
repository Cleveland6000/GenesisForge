#include "chunk_manager.hpp"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <limits>
// chunk_mesh_generator.hpp は ChunkProcessor でのみ使用されるため、ここからは削除可能
// chunk_renderer.hpp は updateChunkRenderData で使用するため残す
#include "chunk_renderer.hpp"
#include "ThreadPool.hpp"

// コンストラクタ
ChunkManager::ChunkManager(int chunkSize, int renderDistanceXZ, unsigned int noiseSeed, float noiseScale,
                           int worldMaxHeight, int groundLevel, int octaves, float lacunarity, float persistence)
    : m_chunkSize(chunkSize), m_renderDistance(renderDistanceXZ),
      // TerrainGenerator を ChunkProcessor に渡す
      m_chunkProcessor(std::make_unique<ChunkProcessor>(chunkSize,
                                                        std::make_unique<TerrainGenerator>(noiseSeed, noiseScale,
                                                                                           worldMaxHeight, groundLevel,
                                                                                           octaves, lacunarity, persistence))),
      m_lastPlayerChunkCoord(std::numeric_limits<int>::max()), m_threadPool(std::make_unique<ThreadPool>(5))
{
    std::cout << "ChunkManager constructor called. ChunkSize: " << m_chunkSize
              << ", RenderDistance: " << m_renderDistance << std::endl;
}

// デストラクタ (変更なし)
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
        loadChunksInArea(currentChunkCoord);
        unloadDistantChunks(currentChunkCoord);
        m_lastPlayerChunkCoord = currentChunkCoord;
    }

    // 完了したチャンク生成タスクの結果を処理
    auto it_chunk_gen = m_pendingChunkGenerations.begin();
    while (it_chunk_gen != m_pendingChunkGenerations.end())
    {
        if (it_chunk_gen->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            glm::ivec3 chunkCoord = it_chunk_gen->first;
            std::shared_ptr<Chunk> newChunk = it_chunk_gen->second.get();

            if (newChunk)
            {
                m_chunks[chunkCoord] = newChunk;
                newChunk->setDirty(true);

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
            it_chunk_gen = m_pendingChunkGenerations.erase(it_chunk_gen);
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
        if (pair.second->isDirty() && m_pendingMeshGenerations.find(pair.first) == m_pendingMeshGenerations.end())
        {
            chunksToProcessMesh.push_back(pair.first);
            pair.second->setDirty(false);
        }
    }

    for (const auto &chunkCoord : chunksToProcessMesh)
    {
        std::shared_ptr<Chunk> chunk = m_chunks[chunkCoord];

        // ChunkProcessor の generateMeshForChunk を非同期で実行
        // this を NeighborChunkProvider* として渡す

        std::cout << "AAA" << std::endl;
        m_pendingMeshGenerations[chunkCoord] = m_threadPool->enqueue(
            &ChunkProcessor::generateMeshForChunk,
            m_chunkProcessor.get(),   // ChunkProcessor のインスタンス
            chunkCoord, chunk, this); // this は NeighborChunkProvider*

        std::cout << "SSS" << std::endl;
    }
    std::cout << "BBB" << std::endl;

    // 完了したメッシュ生成タスクの結果を処理 (OpenGLリソース更新はメインスレッドで行う)
    const int MAX_MESH_UPDATES_PER_FRAME = 1;
    int updatesThisFrame = 0;

    auto it_mesh_gen = m_pendingMeshGenerations.begin();
    while (it_mesh_gen != m_pendingMeshGenerations.end() && updatesThisFrame < MAX_MESH_UPDATES_PER_FRAME)
    {
        if (it_mesh_gen->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            glm::ivec3 chunkCoord = it_mesh_gen->first;
            ChunkMeshData meshData = it_mesh_gen->second.get();

            updateChunkRenderData(chunkCoord, meshData);
            it_mesh_gen = m_pendingMeshGenerations.erase(it_mesh_gen);
            updatesThisFrame++;
        }
        else
        {
            ++it_mesh_gen;
        }
    }
}

// 指定されたワールド座標のチャンクが存在するかどうかをチェックします (変更なし)
bool ChunkManager::hasChunk(const glm::ivec3 &chunkCoord) const
{
    return m_chunks.count(chunkCoord) > 0;
}

// 指定されたチャンク座標のチャンクを取得します (NeighborChunkProvider のオーバーライド)
std::shared_ptr<Chunk> ChunkManager::getChunk(const glm::ivec3 &chunkCoord)
{
    auto it = m_chunks.find(chunkCoord);
    if (it != m_chunks.end())
    {
        return it->second;
    }
    return nullptr;
}

// ワールド座標からチャンク座標を計算するヘルパー関数 (変更なし)
glm::ivec3 ChunkManager::getChunkCoordFromWorldPos(const glm::vec3 &worldPos) const
{
    return glm::ivec3(std::floor(worldPos.x / m_chunkSize),
                      std::floor(worldPos.y / m_chunkSize),
                      std::floor(worldPos.z / m_chunkSize));
}

// プレイヤーを中心としたエリア内のチャンクをロード（存在しない場合は生成）
void ChunkManager::loadChunksInArea(const glm::ivec3 &centerChunkCoord)
{
    for (int y_offset = -m_renderDistance; y_offset <= m_renderDistance; ++y_offset)
    {
        for (int x_offset = -m_renderDistance; x_offset <= m_renderDistance; ++x_offset)
        {
            for (int z_offset = -m_renderDistance; z_offset <= m_renderDistance; ++z_offset)
            {
                // 中心チャンクからの距離を計算 (球形にするための変更点)
                float distance = std::sqrt(static_cast<float>(x_offset * x_offset + y_offset * y_offset + z_offset * z_offset));

                // 距離がレンダリング距離内であればロード
                if (distance <= m_renderDistance)
                {
                    glm::ivec3 chunkCoord = centerChunkCoord + glm::ivec3(x_offset, y_offset, z_offset);
                    if (!hasChunk(chunkCoord) && m_pendingChunkGenerations.find(chunkCoord) == m_pendingChunkGenerations.end())
                    {
                        // ChunkProcessor の generateChunkData を非同期で実行
                        m_pendingChunkGenerations[chunkCoord] = std::async(std::launch::async,
                                                                           &ChunkProcessor::generateChunkData,
                                                                           m_chunkProcessor.get(), chunkCoord);
                    }
                }
            }
        }
    }
}

// 描画距離外に出たチャンクをアンロード (変更なし)
void ChunkManager::unloadDistantChunks(const glm::ivec3 &centerChunkCoord)
{
    std::vector<glm::ivec3> chunksToUnload;
    for (auto const &[coord, chunk] : m_chunks)

    {
        int dist_x = std::abs(coord.x - centerChunkCoord.x);
        int dist_y = std::abs(coord.y - centerChunkCoord.y);
        int dist_z = std::abs(coord.z - centerChunkCoord.z);

        // 球形アンロードの場合も、距離計算を調整
        float distance = std::sqrt(static_cast<float>(dist_x * dist_x + dist_y * dist_y + dist_z * dist_z));

        if (distance > m_renderDistance) // レンダリング距離の外側であればアンロード

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

// OpenGLリソースの更新はメインスレッドで行う (変更なし)
void ChunkManager::updateChunkRenderData(const glm::ivec3 &chunkCoord, const ChunkMeshData &meshData)
{
    auto it = m_chunkRenderData.find(chunkCoord);
    if (it != m_chunkRenderData.end())
    {
        // 既存のVAO, VBO, EBOを削除
        if (it->second.VAO != 0)
            glDeleteVertexArrays(1, &it->second.VAO);
        if (it->second.VBO != 0)
            glDeleteBuffers(1, &it->second.VBO);
        if (it->second.EBO != 0)
            glDeleteBuffers(1, &it->second.EBO);
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
        // メッシュデータが空の場合の処理
    }
}