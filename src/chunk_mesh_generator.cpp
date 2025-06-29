#include "chunk_manager.hpp"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <limits>
#include <glm/glm.hpp> // glm::ivec3 のために必要
#include "chunk_mesh_generator.hpp"

// ChunkMeshGenerator 内で定義されている neighborOffsets を使用するための外部宣言
// chunk_mesh_generator.hpp で定義されていることを前提とします。
// もし定義されていない場合、こちらで定義するか、適切な場所からインポートしてください。
// 例: extern const glm::ivec3 neighborOffsets[6];

// コンストラクタ
// プレイヤーの位置に基づいてチャンクを更新（ロード/アンロード/メッシュ更新）
void ChunkManager::update(const glm::vec3 &playerPosition)
{
    glm::ivec3 currentChunkCoord = getChunkCoordFromWorldPos(playerPosition);

    // プレイヤーのチャンク座標が変わった場合のみ処理を実行
    if (currentChunkCoord != m_lastPlayerChunkCoord)
    {
        loadChunksInArea(currentChunkCoord);
        unloadDistantChunks(currentChunkCoord);
        m_lastPlayerChunkCoord = currentChunkCoord;

        // m_chunksを安全に反復処理し、ダーティなチャンクをメッシュ生成キューに追加
        {
            std::unique_lock<std::mutex> mapLock(m_chunksMutex); // m_chunksへのアクセスを保護
            std::unique_lock<std::mutex> queueLock(m_meshGenerationMutex); // メッシュキューへのアクセスを保護

            for (auto &pair : m_chunks)
            {
                if (pair.second->isDirty())
                {
                    m_meshGenerationQueue.push(pair.first);
                    pair.second->setDirty(false); // キューに入れたらダーティフラグをクリア
                }
            }
            // メッシュキューにタスクが追加されたことをワーカースレッドに通知
            queueLock.unlock(); // ロックは通知前に解放しても良いが、ここでは明示的に。
            m_meshGenerationCondVar.notify_all(); // すべてのワーカースレッドに通知
        } // mapLock はこのスコープを抜けるときに解放される
    }

    // 完了したメッシュデータを処理 (これはメインスレッドで行う)
    processCompletedMeshes();
}

// 指定されたワールド座標のチャンクが存在するかどうかをチェックします
bool ChunkManager::hasChunk(const glm::ivec3 &chunkCoord) const
{
    std::unique_lock<std::mutex> lock(m_chunksMutex); // m_chunksへのアクセスを保護
    return m_chunks.count(chunkCoord) > 0;
}

// 指定されたチャンク座標のチャンクを取得します
std::shared_ptr<Chunk> ChunkManager::getChunk(const glm::ivec3 &chunkCoord)
{
    std::unique_lock<std::mutex> lock(m_chunksMutex); // m_chunksへのアクセスを保護
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
    // ChunkコンストラクタにchunkCoordを渡す
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
    newChunk->setDirty(true); // 新しく生成されたチャンクはダーティなのでメッシュ生成が必要
    return newChunk;
}

// チャンクのメッシュを生成し、レンダリングデータを更新します (この関数はもう直接呼び出されず、ワーカースレッドに移行)
// ただし、ChunkMeshGenerator::generateMesh が呼ばれるために、ChunkManager の getChunk を利用する場合があるので、
// updateChunkMesh は meshGenerationWorker のロジックに統合されます。
void ChunkManager::updateChunkMesh(const glm::ivec3 &chunkCoord, std::shared_ptr<Chunk> chunk)
{
    // この関数は、マルチスレッド化によって直接呼ばれることはなくなります。
    // そのロジックは meshGenerationWorker() に移行します。
    // メッシュ生成のロジックをここに残すのは、ChunkMeshGenerator::generateMesh の引数として渡すためです。

    // これは ChunkMeshGenerator::generateMesh が隣接チャンクを必要とすることを明確にするためのヘルパーです
    // 隣接チャンクを取得 (m_chunksへのアクセスなので、呼び出し側でm_chunksMutexを保護する必要がある)
    const Chunk* neighbor_neg_x = getChunk(glm::ivec3(chunkCoord.x - 1, chunkCoord.y, chunkCoord.z)).get();
    const Chunk* neighbor_pos_x = getChunk(glm::ivec3(chunkCoord.x + 1, chunkCoord.y, chunkCoord.z)).get();
    const Chunk* neighbor_neg_y = getChunk(glm::ivec3(chunkCoord.x, chunkCoord.y - 1, chunkCoord.z)).get();
    const Chunk* neighbor_pos_y = getChunk(glm::ivec3(chunkCoord.x, chunkCoord.y + 1, chunkCoord.z)).get();
    const Chunk* neighbor_neg_z = getChunk(glm::ivec3(chunkCoord.x, chunkCoord.y, chunkCoord.z - 1)).get();
    const Chunk* neighbor_pos_z = getChunk(glm::ivec3(chunkCoord.x, chunkCoord.y, chunkCoord.z + 1)).get();
    
    // ChunkMeshGenerator を使用してメッシュデータを生成します
    ChunkMeshData meshData = ChunkMeshGenerator::generateMesh(
        *chunk,
        neighbor_neg_x, neighbor_pos_x,
        neighbor_neg_y, neighbor_pos_y,
        neighbor_neg_z, neighbor_pos_z
    );

    // 生成されたメッシュデータをレンダリングデータとして保存
    // これはワーカースレッド内では行わず、完了キューを通じてメインスレッドで行う
    // m_chunkRenderData[chunkCoord] = ChunkRenderer::createChunkRenderData(meshData);
}

// 指定された座標範囲内のチャンクをロードします
void ChunkManager::loadChunksInArea(const glm::ivec3 &centerChunkCoord)
{
    // m_chunksへの変更があるのでロック
    std::unique_lock<std::mutex> lock(m_chunksMutex); 
    
    for (int x = -m_renderDistance; x <= m_renderDistance; ++x)
    {
        for (int y = -m_renderDistance; y <= m_renderDistance; ++y)
        {
            for (int z = -m_renderDistance; z <= m_renderDistance; ++z)
            {
                glm::ivec3 currentChunkCoord = glm::ivec3(centerChunkCoord.x + x, centerChunkCoord.y + y, centerChunkCoord.z + z);

                if (m_chunks.count(currentChunkCoord) == 0) // hasChunk() の代わりに直接 m_chunks.count を使用 (ロックを既に持っているため)
                {
                    std::shared_ptr<Chunk> newChunk = generateChunk(currentChunkCoord); // generateChunkは自身の内部でm_chunksMutexを必要としない
                    m_chunks[currentChunkCoord] = newChunk;
                    
                    // 新しいチャンクがロードされたときに、その隣接チャンク（既に存在する場合）もダーティにする
                    // neighborOffsets は ChunkMeshGenerator.hpp で定義されていると仮定
                    for (int i = 0; i < 6; ++i) {
                        glm::ivec3 offset = neighborOffsets[i];
                        glm::ivec3 neighborCoord = currentChunkCoord + offset;
                        // getChunk() は m_chunksMutex を使用しているので、ここでは再ロックは不要
                        // lock_guard ではなく unique_lock を使っているので、ここではすでにロックされている。
                        // getChunk() 内のロックは、この関数外から呼ばれた場合に備えて残す。
                        auto it = m_chunks.find(neighborCoord);
                        if (it != m_chunks.end()) {
                            it->second->setDirty(true);
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
    
    // m_chunksへの読み取りアクセスなのでロック
    std::unique_lock<std::mutex> lock(m_chunksMutex); 

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
    
    // アンロードするチャンクを m_chunks から削除
    for (const auto &coord : chunksToUnload)
    {
        // チャンクがアンロードされるときに、その隣接チャンク（まだ存在する場合）もダーティにする
        for (int i = 0; i < 6; ++i) {
            glm::ivec3 offset = neighborOffsets[i];
            glm::ivec3 neighborCoord = coord + offset;
            auto it = m_chunks.find(neighborCoord); // ロックされた状態で検索
            if (it != m_chunks.end()) {
                it->second->setDirty(true);
            }
        }

        // レンダリングデータを削除 (OpenGLリソースの解放はデストラクタで一括か、メインスレッドで行う)
        // ここではマップからエントリを削除するだけ
        auto renderDataIt = m_chunkRenderData.find(coord);
        if (renderDataIt != m_chunkRenderData.end()) {
             ChunkRenderer::deleteChunkRenderData(renderDataIt->second); // OpenGL リソースを解放
             m_chunkRenderData.erase(renderDataIt);
        }
        
        m_chunks.erase(coord);
    }
} // lock がスコープを抜けるときに解放される

// ワールド座標からチャンク座標を計算します
glm::ivec3 ChunkManager::getChunkCoordFromWorldPos(const glm::vec3 &worldPos) const
{
    int chunkX = static_cast<int>(std::floor(worldPos.x / m_chunkSize));
    int chunkY = static_cast<int>(std::floor(worldPos.y / m_chunkSize));
    int chunkZ = static_cast<int>(std::floor(worldPos.z / m_chunkSize));
    return glm::ivec3(chunkX, chunkY, chunkZ);
}

// ----- マルチスレッド処理のワーカー関数 -----
void ChunkManager::meshGenerationWorker() {
    while (!m_stopWorkers) {
        glm::ivec3 chunkCoord;
        std::shared_ptr<Chunk> chunkToMesh = nullptr;
        
        { // メッシュ生成キューからタスクを取り出すためのロック
            std::unique_lock<std::mutex> lock(m_meshGenerationMutex);
            // キューが空でなく、かつ停止信号が来ていない限り待機
            m_meshGenerationCondVar.wait(lock, [this] {
                return !m_meshGenerationQueue.empty() || m_stopWorkers;
            });

            if (m_stopWorkers) {
                break; // 停止信号を受け取ったらループを抜ける
            }

            chunkCoord = m_meshGenerationQueue.front();
            m_meshGenerationQueue.pop();
        } // lock がスコープを抜けるときに解放される

        // m_chunks からチャンクオブジェクトを取得 (m_chunksMutex で保護)
        {
            std::unique_lock<std::mutex> mapLock(m_chunksMutex);
            auto it = m_chunks.find(chunkCoord);
            if (it != m_chunks.end()) {
                chunkToMesh = it->second;
            }
        } // mapLock がスコープを抜けるときに解放される

        if (chunkToMesh) {
            // メッシュ生成に必要な隣接チャンクの情報を取得する
            // ここでの getChunk() の呼び出しは、m_chunksMutex で保護されている getChunk() を通して行われる。
            // しかし、updateChunkMesh 自体はもう直接呼び出さないため、このロジックは
            // ChunkMeshGenerator::generateMesh に直接必要な引数を渡す形に書き換える。
            
            // 隣接チャンクのポインタを取得 (m_chunksMutex で保護)
            const Chunk* neighbor_neg_x = nullptr;
            const Chunk* neighbor_pos_x = nullptr;
            const Chunk* neighbor_neg_y = nullptr;
            const Chunk* neighbor_pos_y = nullptr;
            const Chunk* neighbor_neg_z = nullptr;
            const Chunk* neighbor_pos_z = nullptr;

            {
                std::unique_lock<std::mutex> mapLock(m_chunksMutex); // m_chunksへのアクセスを保護
                neighbor_neg_x = getChunk(glm::ivec3(chunkCoord.x - 1, chunkCoord.y, chunkCoord.z)).get();
                neighbor_pos_x = getChunk(glm::ivec3(chunkCoord.x + 1, chunkCoord.y, chunkCoord.z)).get();
                neighbor_neg_y = getChunk(glm::ivec3(chunkCoord.x, chunkCoord.y - 1, chunkCoord.z)).get();
                neighbor_pos_y = getChunk(glm::ivec3(chunkCoord.x, chunkCoord.y + 1, chunkCoord.z)).get();
                neighbor_neg_z = getChunk(glm::ivec3(chunkCoord.x, chunkCoord.y, chunkCoord.z - 1)).get();
                neighbor_pos_z = getChunk(glm::ivec3(chunkCoord.x, chunkCoord.y, chunkCoord.z + 1)).get();
            } // mapLock がスコープを抜けるときに解放される

            // メッシュ生成 (計算負荷は高いが、m_chunksを変更しないため、ここではロック不要)
            ChunkMeshData meshData = ChunkMeshGenerator::generateMesh(
                *chunkToMesh,
                neighbor_neg_x, neighbor_pos_x,
                neighbor_neg_y, neighbor_pos_y,
                neighbor_neg_z, neighbor_pos_z
            );

            { // 生成されたメッシュデータを完了キューに追加
                std::unique_lock<std::mutex> lock(m_completedMeshesMutex);
                m_completedMeshes.push({chunkCoord, meshData});
                m_completedMeshesCondVar.notify_one(); // メインスレッドに完了を通知
            }
        } else {
            // チャンクがメッシュ生成キューから取り出された時点で既にアンロードされていた場合
            // (プレイヤーが移動して描画距離外に出たなど)
            // 何もせずスキップ
            // std::cout << "Warning: Chunk " << chunkCoord.x << "," << chunkCoord.y << "," << chunkCoord.z << " not found for mesh generation." << std::endl;
        }
    }
}

// メインスレッドで完了したメッシュデータを処理し、OpenGLリソースを更新します
void ChunkManager::processCompletedMeshes() {
    std::unique_lock<std::mutex> lock(m_completedMeshesMutex);
    while (!m_completedMeshes.empty()) {
        std::pair<glm::ivec3, ChunkMeshData> completedMesh = m_completedMeshes.front();
        m_completedMeshes.pop();
        lock.unlock(); // OpenGL呼び出し中はミューテックスを解放

        // 古いレンダリングデータが存在する場合は解放
        auto it = m_chunkRenderData.find(completedMesh.first);
        if (it != m_chunkRenderData.end()) {
            ChunkRenderer::deleteChunkRenderData(it->second);
        }
        
        // 新しいレンダリングデータを生成し、保存
        // ChunkRenderer::createChunkRenderData は OpenGL 呼び出しを含むため、メインスレッドで行う
        m_chunkRenderData[completedMesh.first] = ChunkRenderer::createChunkRenderData(completedMesh.second);

        lock.lock(); // 次のメッシュ処理のためにミューテックスを再取得
    }
}