#include "chunk_manager.hpp"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <limits>
#include "chunk_renderer.hpp" // ChunkRenderer::createChunkRenderData のために必要


// コンストラクタ
ChunkManager::ChunkManager(int chunkSize, int renderDistanceXZ, unsigned int noiseSeed, float noiseScale,
                           int worldMaxHeight, int groundLevel, int octaves, float lacunarity, float persistence,
                           size_t numWorkerThreads)
    : m_chunkSize(chunkSize), m_renderDistance(renderDistanceXZ),
      m_chunkProcessor(std::make_unique<ChunkProcessor>(chunkSize,
                                                        std::make_unique<TerrainGenerator>(noiseSeed, noiseScale,
                                                                                           worldMaxHeight, groundLevel,
                                                                                           octaves, lacunarity, persistence))),
      m_threadPool(std::make_unique<ThreadPool>(numWorkerThreads)),
      m_lastPlayerChunkCoord(std::numeric_limits<int>::max())
{
    std::cout << "ChunkManager constructor called. ChunkSize: " << m_chunkSize
              << ", RenderDistanceXZ: " << m_renderDistance
              << ", WorkerThreads: " << numWorkerThreads << std::endl;
}

// デストラクタ
ChunkManager::~ChunkManager()
{
    std::cout << "ChunkManager destructor called." << std::endl;
    // ThreadPool のデストラクタがスレッドの終了を適切に処理する
    // ここで明示的にタスクの完了を待つ必要はない
    m_chunkRenderData.clear();
}

// プレイヤーの位置に基づいてチャンクを更新（ロード/アンロード/メッシュ更新）
void ChunkManager::update(const glm::vec3 &playerPosition)
{
    glm::ivec3 currentChunkCoord = getChunkCoordFromWorldPos(playerPosition);

    // プレイヤーが新しいチャンクに移動した場合、チャンクのロード/アンロードを更新
    if (currentChunkCoord != m_lastPlayerChunkCoord)
    {
        std::cout << "ChunkManager: Player moved to new chunk: "
                  << currentChunkCoord.x << ", " << currentChunkCoord.y << ", " << currentChunkCoord.z << std::endl;
        loadChunksInArea(currentChunkCoord);
        unloadDistantChunks(currentChunkCoord);
        m_lastPlayerChunkCoord = currentChunkCoord;
    }

    // 完了したチャンクデータ生成タスクを処理
    for (auto it = m_pendingChunkGenerations.begin(); it != m_pendingChunkGenerations.end(); ) {
        if (it->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            glm::ivec3 chunkCoord = it->first;
            std::shared_ptr<Chunk> newChunk = it->second.get();
            if (newChunk) {
                // デバッグ出力
                std::cout << "DEBUG: Chunk data generated for " << chunkCoord.x << "," << chunkCoord.y << "," << chunkCoord.z
                          << ". Is empty? " << (newChunk->getVoxels().empty() ? "YES" : "NO") << std::endl;
                m_chunks[chunkCoord] = newChunk;
                // チャンクデータが生成されたら、メッシュ生成タスクを投入
                m_pendingMeshGenerations[chunkCoord] = m_threadPool->enqueue(
                    [this, chunkCoord, newChunk]() {
                        // ここで m_chunkMutex を使用して m_chunks へのアクセスを保護
                        // generateMeshForChunk の中で getNeighbor が呼ばれるため、m_chunkMutex が必要
                        // ただし、newChunk はこのラムダ内で安全にアクセスできる
                        return m_chunkProcessor->generateMeshForChunk(chunkCoord, newChunk, this);
                    });
            } else {
                std::cerr << "ERROR: Failed to generate chunk data for " << chunkCoord.x << "," << chunkCoord.y << "," << chunkCoord.z << std::endl;
            }
            it = m_pendingChunkGenerations.erase(it);
        } else {
            ++it;
        }
    }

    // 完了したメッシュ生成タスクを処理
    for (auto it = m_pendingMeshGenerations.begin(); it != m_pendingMeshGenerations.end(); ) {
        if (it->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            glm::ivec3 chunkCoord = it->first;
            ChunkMeshData meshData = it->second.get();
            if (!meshData.vertices.empty()) {
                // デバッグ出力
                std::cout << "DEBUG: Mesh generated for " << chunkCoord.x << "," << chunkCoord.y << "," << chunkCoord.z
                          << ". Vertices: " << meshData.vertices.size() << ", Indices: " << meshData.indices.size() << std::endl;
                updateChunkRenderData(chunkCoord, meshData); // OpenGLリソースの更新
            } else {
                // デバッグ出力
                std::cout << "DEBUG: Empty mesh generated for " << chunkCoord.x << "," << chunkCoord.y << "," << chunkCoord.z << std::endl;
                // メッシュが空の場合でも、既存の描画データをクリアするために updateChunkRenderData を呼ぶことも検討
                // updateChunkRenderData(chunkCoord, meshData);
            }
            it = m_pendingMeshGenerations.erase(it);
        } else {
            ++it;
        }
    }
}

// ワールド座標からチャンク座標を取得
glm::ivec3 ChunkManager::getChunkCoordFromWorldPos(const glm::vec3 &worldPos) const
{
    // C++ の / 演算子は負の数に対して切り捨てを行うため、床関数を使用
    return glm::ivec3(
        static_cast<int>(std::floor(worldPos.x / m_chunkSize)),
        static_cast<int>(std::floor(worldPos.y / m_chunkSize)),
        static_cast<int>(std::floor(worldPos.z / m_chunkSize)));
}

// 指定されたエリア内のチャンクをロードする（またはロードをスケジュールする）
void ChunkManager::loadChunksInArea(const glm::ivec3 &centerChunkCoord)
{
    for (int x_offset = -m_renderDistance; x_offset <= m_renderDistance; ++x_offset)
    {
        for (int y_offset = -m_renderDistance; y_offset <= m_renderDistance; ++y_offset) // Y軸方向もロード対象
        {
            for (int z_offset = -m_renderDistance; z_offset <= m_renderDistance; ++z_offset)
            {
                glm::ivec3 chunkCoord = centerChunkCoord + glm::ivec3(x_offset, y_offset, z_offset);

                // チャンクがまだロードされておらず、生成が保留されていない場合
                if (!hasChunk(chunkCoord) && m_pendingChunkGenerations.find(chunkCoord) == m_pendingChunkGenerations.end())
                {
                    // スレッドプールにチャンクデータ生成タスクを投入
                    m_pendingChunkGenerations[chunkCoord] = m_threadPool->enqueue(
                        [this, chunkCoord]() { return m_chunkProcessor->generateChunkData(chunkCoord); });
                }
            }
        }
    }
}

// 遠すぎるチャンクをアンロードする
void ChunkManager::unloadDistantChunks(const glm::ivec3 &centerChunkCoord)
{
    // アンロード対象のチャンクを一時的に保存するリスト
    std::vector<glm::ivec3> chunksToUnload;

    // 現在ロードされているすべてのチャンクをイテレート
    for (const auto &pair : m_chunks)
    {
        glm::ivec3 chunkCoord = pair.first;
        // チャンクが現在のレンダリング距離外にあるかチェック
        if (glm::distance(static_cast<glm::vec3>(chunkCoord), static_cast<glm::vec3>(centerChunkCoord)) > m_renderDistance + 0.5f)
        {
            chunksToUnload.push_back(chunkCoord);
        }
    }

    // リストに保存されたチャンクをアンロード
    for (const glm::ivec3 &coord : chunksToUnload)
    {
        // まず、このチャンクを参照している可能性のある隣接チャンクをダーティとしてマークする
        // これにより、そのメッシュが再生成されることを保証
        static const std::array<glm::ivec3, 6> neighborOffsets = {
            glm::ivec3(0, 0, -1), // Back face (Z-)
            glm::ivec3(0, 0, 1),  // Front face (Z+)
            glm::ivec3(-1, 0, 0), // Left face (X-)
            glm::ivec3(1, 0, 0),  // Right face (X+)
            glm::ivec3(0, -1, 0), // Bottom face (Y-)
            glm::ivec3(0, 1, 0)   // Top face (Y+)
        };

        for (int i = 0; i < 6; ++i)
        {
            glm::ivec3 offset = neighborOffsets[i];
            glm::ivec3 neighborCoord = coord + offset;
            std::shared_ptr<Chunk> neighborChunk = getChunk(neighborCoord); // getChunk は内部でロックを取る
            if (neighborChunk)
            {
                neighborChunk->setDirty(true);
            }
        }

        // m_chunkRenderData はメインスレッドからのみアクセスされると仮定されているが、
        // 明示的に保護するならここもロックが必要。ここでは省略。
        m_chunkRenderData.erase(coord); 
        m_chunks.erase(coord);
    }
}

// チャンクが存在するかどうかを安全に確認
bool ChunkManager::hasChunk(const glm::ivec3 &chunkCoord) const
{
    std::lock_guard<std::mutex> lock(m_chunkMutex); // m_chunks へのアクセスを保護
    return m_chunks.count(chunkCoord) > 0;
}

// チャンクを安全に取得
std::shared_ptr<Chunk> ChunkManager::getChunk(const glm::ivec3 &chunkCoord)
{
    std::lock_guard<std::mutex> lock(m_chunkMutex); // m_chunks へのアクセスを保護
    auto it = m_chunks.find(chunkCoord);
    if (it != m_chunks.end())
    {
        return it->second;
    }
    return nullptr;
}

// OpenGLリソースの更新はメインスレッドで行う
// この関数はメインスレッドからのみ呼び出されるため、m_chunkRenderData へのロックは不要と仮定します。
// もし他のスレッドからアクセスされる可能性がある場合は、ここにもロックが必要です。
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

    if (!meshData.vertices.empty() && !meshData.indices.empty())
    {
        // 新しいOpenGLリソースを生成
        m_chunkRenderData[chunkCoord] = ChunkRenderer::createChunkRenderData(meshData);
    }
    // メッシュデータが空の場合は、チャンクの描画データを削除するだけ（上で既に削除されている）
}