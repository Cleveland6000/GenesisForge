#include "chunk_manager.hpp"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <limits>
#include <thread> // std::thread::hardware_concurrency() のために必要
#include "chunk_mesh_generator.hpp" // neighborOffsets が定義されるため、このインクルードで解決されます

// コンストラクタ
ChunkManager::ChunkManager(int chunkSize, int renderDistanceXZ, unsigned int noiseSeed, float noiseScale,
                           int worldMaxHeight, int groundLevel, int octaves, float lacunarity, float persistence)
    : m_chunkSize(chunkSize), m_renderDistance(renderDistanceXZ),
      m_terrainGenerator(std::make_unique<TerrainGenerator>(noiseSeed, noiseScale, worldMaxHeight, groundLevel,
                                                             octaves, lacunarity, persistence)),
      m_lastPlayerChunkCoord(std::numeric_limits<int>::max()),
      m_threadPool(std::make_unique<ThreadPool>(std::thread::hardware_concurrency() > 0 ? std::thread::hardware_concurrency() : 2))
{
    std::cout << "ChunkManager constructor called. ChunkSize: " << m_chunkSize
              << ", RenderDistanceXZ: " << m_renderDistance << std::endl;
}

// デストラクタ
ChunkManager::~ChunkManager()
{
    std::cout << "ChunkManager destructor called." << std::endl;
    // m_threadPool は unique_ptr なので、スコープを抜けるときに自動的にデストラクタが呼び出され、
    // ThreadPoolのデストラクタ内で全てのスレッドが安全に終了するまで待機します。
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

    // ★変更: 完了した地形生成タスクを処理
    // m_pendingTerrainTasks はマップであり、イテレータの無効化を避けるためにコピーまたは一時的なリストを使う
    // または、イテレータを安全に管理しながら削除する
    for (auto it = m_pendingTerrainTasks.begin(); it != m_pendingTerrainTasks.end(); )
    {
        // ノンブロッキングでタスクの完了をチェック
        if (it->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            glm::ivec3 chunkCoord = it->first;
            std::shared_ptr<Chunk> chunk = it->second.get(); // 結果を取得（既にreadyなのでブロックされない）

            // メインスレッドからのみアクセスされる m_chunks にチャンクを追加
            // ここでチャンクの挿入を保護する必要がある場合は、m_chunkGenerationMutex を使用します
            // ただし、update関数はメインスレッドで実行される前提であれば、m_chunksへの直接アクセスは問題ありません
            m_chunks[chunkCoord] = chunk;
            
            // 地形生成が完了したチャンクの隣接チャンクをダーティにする
            // ここで getChunk を呼び出すため、m_chunks が更新された後に実行されることが重要
            for (int i = 0; i < 6; ++i) {
                glm::ivec3 offset = neighborOffsets[i];
                glm::ivec3 neighborCoord = chunkCoord + offset;
                std::shared_ptr<Chunk> neighborChunk = getChunk(neighborCoord);
                if (neighborChunk) {
                    neighborChunk->setDirty(true);
                }
            }

            // メッシュ生成タスクをスレッドプールに投入
            // updateChunkMesh は ChunkMeshData を返すように変更されています
            auto mesh_future = m_threadPool->enqueue([this, chunkCoord, chunk]() {
                // updateChunkMesh 内で getChunk が使われる可能性があるため、
                // 隣接チャンクがまだ生成されていない場合でも安全に動作することを確認してください。
                // (例えば、getChunkがnullptrを返す、ChunkMeshGeneratorがnullptrを処理できるなど)
                return updateChunkMesh(chunkCoord, chunk);
            });
            // m_pendingMeshTasks にタスクを追加（メインスレッドからアクセスなのでロックは不要）
            m_pendingMeshTasks[chunkCoord] = std::move(mesh_future);

            // 処理済みタスクをマップから削除
            it = m_pendingTerrainTasks.erase(it);
        }
        else
        {
            ++it; // 次のタスクへ
        }
    }

    // ★変更: 完了したメッシュ生成タスクを処理（OpenGLリソースの作成）
    // m_pendingMeshTasks から直接 future を取得して処理する
    for (auto it = m_pendingMeshTasks.begin(); it != m_pendingMeshTasks.end(); )
    {
        if (it->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready) // ノンブロッキングチェック
        {
            glm::ivec3 chunkCoord = it->first;
            ChunkMeshData meshData = it->second.get(); // 結果を取得（既にreadyなのでブロックされない）

            // OpenGLの呼び出しはメインスレッドで行う必要があります！
            // ChunkRenderer::createChunkRenderData は OpenGL のコンテキストを必要とします。
            m_chunkRenderData[chunkCoord] = ChunkRenderer::createChunkRenderData(meshData);

            // メッシュ生成が完了したチャンクのダーティフラグをリセット
            auto chunkIt = m_chunks.find(chunkCoord);
            if (chunkIt != m_chunks.end()) {
                chunkIt->second->setDirty(false); // メッシュ生成が完了したのでダーティフラグを解除
            }

            // 処理済みタスクをマップから削除
            it = m_pendingMeshTasks.erase(it);
        }
        else
        {
            ++it; // 次のタスクへ
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
// ★変更: この関数はスレッドプールによってバックグラウンドで呼び出されるようになります
std::shared_ptr<Chunk> ChunkManager::generateChunk(const glm::ivec3 &chunkCoord)
{
    // std::cout << "Generating terrain for chunk: " << chunkCoord.x << ", " << chunkCoord.y << ", " << chunkCoord.z << std::endl;
    std::shared_ptr<Chunk> newChunk = std::make_shared<Chunk>(m_chunkSize);

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
    newChunk->setDirty(true); // 地形生成が終わったらメッシュ生成が必要なのでダーティにする
    return newChunk;
}

// チャンクのメッシュを生成し、レンダリングデータを更新します
// ★変更: この関数はスレッドプールによってバックグラウンドで呼び出されるようになります
//         OpenGL呼び出しはここから削除し、メインスレッドに移動します。
ChunkMeshData ChunkManager::updateChunkMesh(const glm::ivec3 &chunkCoord, std::shared_ptr<Chunk> chunk)
{
    // 隣接チャンクを取得 (ここではチャンクのボクセルデータのみ必要なので、const Chunk* で十分)
    // 注意: 非同期メッシュ生成の場合、隣接チャンクがまだロードされていない、
    // または地形生成中の可能性があるため、注意が必要です。
    // そのため、ここではgetChunkを呼び出していますが、完全に非同期化する際は、
    // 隣接チャンクのボクセルデータもスレッドに安全に渡すか、
    // メッシュ生成の前にそれらのチャンクが完全に存在することを保証するロジックが必要です。
    // この例では、getChunkが安全にnullptrを返し、ChunkMeshGeneratorがnullptrを処理できることを前提としています。
    const Chunk* neighbor_neg_x = hasChunk(glm::ivec3(chunkCoord.x - 1, chunkCoord.y, chunkCoord.z)) ? getChunk(glm::ivec3(chunkCoord.x - 1, chunkCoord.y, chunkCoord.z)).get() : nullptr;
    const Chunk* neighbor_pos_x = hasChunk(glm::ivec3(chunkCoord.x + 1, chunkCoord.y, chunkCoord.z)) ? getChunk(glm::ivec3(chunkCoord.x + 1, chunkCoord.y, chunkCoord.z)).get() : nullptr;
    const Chunk* neighbor_neg_y = hasChunk(glm::ivec3(chunkCoord.x, chunkCoord.y - 1, chunkCoord.z)) ? getChunk(glm::ivec3(chunkCoord.x, chunkCoord.y - 1, chunkCoord.z)).get() : nullptr;
    const Chunk* neighbor_pos_y = hasChunk(glm::ivec3(chunkCoord.x, chunkCoord.y + 1, chunkCoord.z)) ? getChunk(glm::ivec3(chunkCoord.x, chunkCoord.y + 1, chunkCoord.z)).get() : nullptr;
    const Chunk* neighbor_neg_z = hasChunk(glm::ivec3(chunkCoord.x, chunkCoord.y, chunkCoord.z - 1)) ? getChunk(glm::ivec3(chunkCoord.x, chunkCoord.y, chunkCoord.z - 1)).get() : nullptr;
    const Chunk* neighbor_pos_z = hasChunk(glm::ivec3(chunkCoord.x, chunkCoord.y, chunkCoord.z + 1)) ? getChunk(glm::ivec3(chunkCoord.x, chunkCoord.y, chunkCoord.z + 1)).get() : nullptr;
    
    // ChunkMeshGenerator を使用してメッシュデータを生成します
    ChunkMeshData meshData = ChunkMeshGenerator::generateMesh(
        *chunk,
        neighbor_neg_x, neighbor_pos_x,
        neighbor_neg_y, neighbor_pos_y,
        neighbor_neg_z, neighbor_pos_z
    );

    // ★重要: ここで直接 OpenGL の呼び出しを削除します。
    // m_chunkRenderData[chunkCoord] = ChunkRenderer::createChunkRenderData(meshData);

    // m_generatedMeshes は今後使用しないため削除
    // {
    //     std::lock_guard<std::mutex> lock(m_meshGenerationMutex);
    //     m_generatedMeshes.push({chunkCoord, meshData});
    // }

    return meshData; // メッシュデータを返すように変更
}

// 指定された座標範囲内のチャンクをロードします
void ChunkManager::loadChunksInArea(const glm::ivec3 &centerChunkCoord)
{
    for (int x = -m_renderDistance; x <= m_renderDistance; ++x)
    {
        for (int y = -m_renderDistance; y <= m_renderDistance; ++y) // Y軸方向の描画距離も考慮
        {
            for (int z = -m_renderDistance; z <= m_renderDistance; ++z)
            {
                glm::ivec3 currentChunkCoord = glm::ivec3(centerChunkCoord.x + x, centerChunkCoord.y + y, centerChunkCoord.z + z);

                // チャンクが存在しない、かつ地形生成タスクがキューにない場合にのみ地形生成をトリガー
                if (!hasChunk(currentChunkCoord) && m_pendingTerrainTasks.find(currentChunkCoord) == m_pendingTerrainTasks.end())
                {
                    // 非同期で地形生成タスクをスレッドプールに投入
                    auto future = m_threadPool->enqueue([this, currentChunkCoord]() {
                        return generateChunk(currentChunkCoord);
                    });
                    m_pendingTerrainTasks[currentChunkCoord] = std::move(future);

                    // ここでは隣接チャンクをダーティに設定しません。
                    // チャンクが完全にロードされ、m_chunksに追加された後（update関数内）で処理します。
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
        // この処理は、チャンクが完全にアンロードされる直前に行うのが適切
        for (int i = 0; i < 6; ++i) {
            glm::ivec3 offset = neighborOffsets[i];
            glm::ivec3 neighborCoord = coord + offset;
            std::shared_ptr<Chunk> neighborChunk = getChunk(neighborCoord);
            if (neighborChunk) {
                neighborChunk->setDirty(true); // 隣接チャンクのメッシュを再生成させるためにダーティに
            }
        }

        m_chunkRenderData.erase(coord);
        m_chunks.erase(coord);
        // ★追加: 終了していない保留中のタスクもクリーンアップ (必須)
        // erase は要素が存在しない場合でも安全に動作します
        m_pendingTerrainTasks.erase(coord);
        m_pendingMeshTasks.erase(coord);
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