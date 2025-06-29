#include "chunk_manager.hpp"
#include <iostream>
#include <chrono>           // 時間計測に使用する場合は残す
#include <algorithm>        // std::abs のために必要
#include <limits>           // std::numeric_limits のために必要
#include <unordered_set>    // m_chunksPendingMeshGeneration のために必要
#include "chunk_mesh_generator.hpp"

// コンストラクタ
ChunkManager::ChunkManager(int chunkSize, int renderDistanceXZ, unsigned int noiseSeed, float noiseScale,
                           int worldMaxHeight, int groundLevel, int octaves, float lacunarity, float persistence)
    : m_chunkSize(chunkSize), m_renderDistance(renderDistanceXZ),
      m_terrainGenerator(std::make_unique<TerrainGenerator>(noiseSeed, noiseScale, worldMaxHeight, groundLevel,
                                                             octaves, lacunarity, persistence)),
      m_lastPlayerChunkCoord(std::numeric_limits<int>::max()),
      // スレッドプールを初期化。利用可能なハードウェアスレッド数から1を引いた数（メインスレッド用）を使う。最低1スレッド。
      m_threadPool(std::thread::hardware_concurrency() > 0 ? std::thread::hardware_concurrency() - 1 : 1) 
{
    std::cout << "ChunkManager constructor called. ChunkSize: " << m_chunkSize
              << ", RenderDistanceXZ: " << m_renderDistance << std::endl;
    std::cout << "ThreadPool initialized with " << (std::thread::hardware_concurrency() > 0 ? std::thread::hardware_concurrency() - 1 : 1) << " threads.\n";
}

// デストラクタ
ChunkManager::~ChunkManager()
{
    std::cout << "ChunkManager destructor called." << std::endl;
    // デストラクタでミューテックスをロックすると、他のスレッドがブロックされている場合にデッドロックを引き起こす可能性があるため、注意が必要です。
    // アプリケーション終了時には通常、他のスレッドは停止しているはずですが、完全に安全性を確保するためには、
    // アプリケーション終了前にスレッドプールをシャットダウンするロジックを確実に入れるべきです。
    // m_chunksMutex をロックせずに m_chunkRenderData をクリアする。
    // m_chunks は shared_ptr なので、参照カウントが0になれば自動的に解放されます。
    // m_chunkRenderData のクリアは OpenGL リソースの解放を含む可能性があり、メインスレッドで行う必要があります。
    m_chunkRenderData.clear(); // 保持しているレンダリングデータをクリアします
}

// プレイヤーの位置に基づいてチャンクを更新（ロード/アンロード/メッシュ更新）
void ChunkManager::update(const glm::vec3 &playerPosition)
{
    std::cout << "DEBUG: ChunkManager::update - Start\n";
    glm::ivec3 currentChunkCoord = getChunkCoordFromWorldPos(playerPosition);

    if (currentChunkCoord != m_lastPlayerChunkCoord)
    {
        std::cout << "DEBUG: ChunkManager::update - Player chunk changed to (" 
                  << currentChunkCoord.x << "," << currentChunkCoord.y << "," << currentChunkCoord.z << "), loading/unloading chunks...\n";
        
        loadChunksInArea(currentChunkCoord);
        std::cout << "DEBUG: ChunkManager::update - loadChunksInArea finished.\n";
        
        unloadDistantChunks(currentChunkCoord);
        std::cout << "DEBUG: ChunkManager::update - unloadDistantChunks finished.\n";
        
        m_lastPlayerChunkCoord = currentChunkCoord;
    }
    else {
        std::cout << "DEBUG: ChunkManager::update - Player chunk not changed. Current: ("
                  << currentChunkCoord.x << "," << currentChunkCoord.y << "," << currentChunkCoord.z << ")\n";
    }

    std::cout << "DEBUG: ChunkManager::update - Checking dirty chunks for mesh generation...\n";
    {
        std::lock_guard<std::mutex> lock(m_chunksMutex); // m_chunksへのアクセスを保護
        for (auto &pair : m_chunks)
        {
            glm::ivec3 chunkCoord = pair.first;
            std::shared_ptr<Chunk> chunk = pair.second;

            if (chunk->isDirty())
            {
                // m_chunksPendingMeshGeneration は m_meshQueueMutex で保護されているので、ここではロックしない
                // requestChunkMeshUpdate 内部で m_meshQueueMutex をロックしてチェック＆挿入する
                bool isPending;
                {
                    std::lock_guard<std::mutex> queue_lock(m_meshQueueMutex);
                    isPending = m_chunksPendingMeshGeneration.count(chunkCoord) > 0;
                }

                if (!isPending)
                {
                    std::cout << "DEBUG: ChunkManager::update - Requesting mesh update for chunk: (" << chunkCoord.x << "," << chunkCoord.y << "," << chunkCoord.z << ")\n";
                    requestChunkMeshUpdate(chunkCoord, chunk);
                    chunk->setDirty(false); // ダーティフラグをリセット (メッシュ生成が開始されたことを示す)
                } else {
                    std::cout << "DEBUG: ChunkManager::update - Chunk (" << chunkCoord.x << "," << chunkCoord.y << "," << chunkCoord.z << ") is dirty but mesh generation is already pending.\n";
                }
            }
        }
    }
    std::cout << "DEBUG: ChunkManager::update - Dirty chunk check finished.\n";

    std::cout << "DEBUG: ChunkManager::update - Processing async mesh results...\n";
    processAsyncMeshResults();
    std::cout << "DEBUG: ChunkManager::update - processAsyncMeshResults finished.\n";
    std::cout << "DEBUG: ChunkManager::update - End\n";
}

// 指定されたワールド座標のチャンクが存在するかどうかをチェックします
bool ChunkManager::hasChunk(const glm::ivec3 &chunkCoord) const
{
    std::lock_guard<std::mutex> lock(m_chunksMutex); // m_chunksへのアクセスを保護
    return m_chunks.count(chunkCoord) > 0;
}

// 指定されたチャンク座標のチャンクを取得します
std::shared_ptr<Chunk> ChunkManager::getChunk(const glm::ivec3 &chunkCoord)
{
    std::lock_guard<std::mutex> lock(m_chunksMutex); // m_chunksへのアクセスを保護
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
    // コンソール出力はデバッグ用です
    // std::cout << "Generating chunk at: (" << chunkCoord.x << ", " << chunkCoord.y << ", " << chunkCoord.z << ")\n";

    std::shared_ptr<Chunk> newChunk = std::make_shared<Chunk>(m_chunkSize);

    if (!m_terrainGenerator)
    {
        std::cerr << "Error: TerrainGenerator instance is not initialized in ChunkManager.\n";
        return newChunk;
    }

    // 高さマップを計算 (X-Z平面の各座標での地形の高さ)
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

    // チャンクのボクセルデータを一時的に保持するベクトルです
    // これを計算し終えてからChunk::setVoxelsで一括設定します
    std::vector<bool> tempVoxels(m_chunkSize * m_chunkSize * m_chunkSize);

    // 各ボクセルの状態を設定します
    // キャッシュ効率を考慮し、Z, Y, X の順でループしています
    for (int z = 0; z < m_chunkSize; ++z)
    {
        for (int y = 0; y < m_chunkSize; ++y)
        {
            for (int x = 0; x < m_chunkSize; ++x)
            {
                float worldY = (float)y + (float)chunkCoord.y * m_chunkSize; // グローバルY座標

                bool isSolid;
                if (worldY < m_terrainGenerator->getGroundLevel())
                {
                    isSolid = true; // 地面レベルより下は常にソリッドです
                }
                else
                {
                    // チャンク内のローカルX, Z座標に対応する地形の高さです
                    int terrainHeightAtXZ = heightMap[x + z * m_chunkSize];
                    isSolid = (worldY < terrainHeightAtXZ); // 地形より下はソリッドです
                }

                // 計算したボクセルデータを一時ベクトルに格納します
                size_t index = static_cast<size_t>(x + y * m_chunkSize + z * m_chunkSize * m_chunkSize);
                tempVoxels[index] = isSolid;
            }
        }
    }

    // 一時ベクトルに格納された全ボクセルデータをチャンクに一括設定します
    newChunk->setVoxels(tempVoxels);
    newChunk->setDirty(true); // ボクセルデータが設定されたのでダーティフラグを立てます
    return newChunk;
}

// メッシュ生成を非同期で要求する関数
void ChunkManager::requestChunkMeshUpdate(const glm::ivec3 &chunkCoord, std::shared_ptr<Chunk> chunk)
{
    // このチャンクが既にキューに入っているか確認（重複防止）
    {
        // m_chunksPendingMeshGeneration へのアクセスを保護
        std::lock_guard<std::mutex> lock(m_meshQueueMutex); 
        if (m_chunksPendingMeshGeneration.count(chunkCoord)) {
            // 既にキューに入っているか処理中であれば、何もしない（ここでは既に上記でチェックしているので念のため）
            return; 
        }
        // 新しくキューに入れるチャンクとしてマーク
        m_chunksPendingMeshGeneration.insert(chunkCoord);
    }
    
    // 隣接チャンクのshared_ptrを格納する配列を構築
    // これらをキャプチャすることで、ワーカースレッドがアクセス中にチャンクが破棄されるのを防ぎます。
    std::array<std::shared_ptr<const Chunk>, 6> adjacentChunksShared;
    adjacentChunksShared[0] = getChunk(glm::ivec3(chunkCoord.x, chunkCoord.y, chunkCoord.z - 1)); // Z- (奥)
    adjacentChunksShared[1] = getChunk(glm::ivec3(chunkCoord.x, chunkCoord.y, chunkCoord.z + 1)); // Z+ (手前)
    adjacentChunksShared[2] = getChunk(glm::ivec3(chunkCoord.x - 1, chunkCoord.y, chunkCoord.z)); // X- (左)
    adjacentChunksShared[3] = getChunk(glm::ivec3(chunkCoord.x + 1, chunkCoord.y, chunkCoord.z)); // X+ (右)
    adjacentChunksShared[4] = getChunk(glm::ivec3(chunkCoord.x, chunkCoord.y - 1, chunkCoord.z)); // Y- (下)
    adjacentChunksShared[5] = getChunk(glm::ivec3(chunkCoord.x, chunkCoord.y + 1, chunkCoord.z)); // Y+ (上)

    std::cout << "DEBUG: requestChunkMeshUpdate - Enqueuing task for chunk: (" << chunkCoord.x << "," << chunkCoord.y << "," << chunkCoord.z << ")\n";
    // スレッドプールにメッシュ生成タスクをエンキューします。
    // ラムダ関数はバックグラウンドスレッドで実行されます。
    // chunk と adjacentChunksShared を値でキャプチャすることで、shared_ptr の参照カウントが増え、
    // メッシュ生成中にチャンクオブジェクトが有効であることが保証されます。
    m_threadPool.enqueue([this, chunkCoord, chunk, adjacentChunksShared]() {
        std::cout << "DEBUG THREAD: Mesh generation task started for chunk: (" << chunkCoord.x << "," << chunkCoord.y << "," << chunkCoord.z << ")\n";

        // ここで shared_ptr<const Chunk> から生ポインタの配列を構築し、generateMesh に渡す
        std::array<const Chunk*, 6> rawAdjacentChunks;
        for(int i = 0; i < 6; ++i) {
            rawAdjacentChunks[i] = adjacentChunksShared[i].get(); // nullptr の場合はそのまま nullptr が入る
        }
        
        // バックグラウンドスレッドでメッシュを生成
        // chunk は shared_ptr なので、直接逆参照して Chunk オブジェクトを渡す
        ChunkMeshData meshData = ChunkMeshGenerator::generateMesh(*chunk, rawAdjacentChunks);
        
        std::cout << "DEBUG THREAD: Mesh generated for chunk: (" << chunkCoord.x << "," << chunkCoord.y << "," << chunkCoord.z << "), pushing to queue.\n";

        // 生成されたメッシュデータをメインスレッドに渡すキューに追加
        // このキューへのアクセスはスレッドセーフにする必要があります。
        {
            std::lock_guard<std::mutex> lock(m_meshQueueMutex);
            m_generatedMeshQueue.push({chunkCoord, meshData});
        }
        // メッシュ生成タスクが完了したので、保留中セットから削除
        // m_chunksPendingMeshGenerationも保護
        {
            std::lock_guard<std::mutex> lock(m_meshQueueMutex); 
            m_chunksPendingMeshGeneration.erase(chunkCoord);
        }
        std::cout << "DEBUG THREAD: Task finished for chunk: (" << chunkCoord.x << "," << chunkCoord.y << "," << chunkCoord.z << ")\n";
    });
}

// 生成済みメッシュをメインスレッドで処理し、GPUにアップロードする関数
void ChunkManager::processAsyncMeshResults()
{
    std::cout << "DEBUG: processAsyncMeshResults - Attempting to lock queue mutex.\n";
    
    // m_meshQueueMutex と m_chunksMutex の両方を安全にロック
    std::unique_lock<std::mutex> lock_queue(m_meshQueueMutex, std::defer_lock);
    std::unique_lock<std::mutex> lock_chunks(m_chunksMutex, std::defer_lock);
    
    // デッドロックを回避しながら両方をロック
    std::lock(lock_queue, lock_chunks); 

    std::cout << "DEBUG: processAsyncMeshResults - Both mutexes locked, processing " << m_generatedMeshQueue.size() << " meshes.\n";
    int processedCount = 0;
    while (!m_generatedMeshQueue.empty())
    {
        auto &pair = m_generatedMeshQueue.front();
        glm::ivec3 chunkCoord = pair.first;
        ChunkMeshData meshData = pair.second; // メッシュデータはコピーされる

        // chunkExists のチェックはロック内で安全に行える
        bool chunkExists = m_chunks.count(chunkCoord) > 0;

        if (chunkExists) 
        {
            std::cout << "DEBUG: processAsyncMeshResults - Processing mesh for chunk: (" << chunkCoord.x << "," << chunkCoord.y << "," << chunkCoord.z << ")\n";
            // レンダリングデータを更新します（GPUにアップロード）
            m_chunkRenderData[chunkCoord] = ChunkRenderer::createChunkRenderData(meshData);
            std::cout << "DEBUG: processAsyncMeshResults - Mesh uploaded for chunk: (" << chunkCoord.x << "," << chunkCoord.y << "," << chunkCoord.z << ")\n";
        } else {
            std::cout << "DEBUG: processAsyncMeshResults - Skipping mesh for unloaded chunk: (" << chunkCoord.x << "," << chunkCoord.y << "," << chunkCoord.z << ")\n";
        }
        m_generatedMeshQueue.pop();
        processedCount++;
    }
    std::cout << "DEBUG: processAsyncMeshResults - Processed " << processedCount << " meshes, queue empty.\n";
} // unique_lo

// 指定された座標範囲内のチャンクをロードします
void ChunkManager::loadChunksInArea(const glm::ivec3 &centerChunkCoord)
{
    std::cout << "DEBUG: loadChunksInArea - Start for center: (" << centerChunkCoord.x << "," << centerChunkCoord.y << "," << centerChunkCoord.z << ")\n";
    // ロードするチャンクのリストを一時的に作成
    std::vector<glm::ivec3> chunksToLoad;
    {
        std::lock_guard<std::mutex> lock(m_chunksMutex); // hasChunkがm_chunksMutexをロックするため、ここで一括ロック
        for (int x = -m_renderDistance; x <= m_renderDistance; ++x)
        {
            for (int y = -m_renderDistance; y <= m_renderDistance; ++y)
            {
                for (int z = -m_renderDistance; z <= m_renderDistance; ++z)
                {
                    glm::ivec3 currentChunkCoord = glm::ivec3(centerChunkCoord.x + x, centerChunkCoord.y + y, centerChunkCoord.z + z);
                    if (!m_chunks.count(currentChunkCoord)) // 直接m_chunksにアクセスして重複チェック (hasChunkは既にロックしているので不要)
                    {
                        chunksToLoad.push_back(currentChunkCoord);
                    }
                }
            }
        }
    } // ロック解放

    // 実際にチャンクをマップに追加（このブロック全体を保護）
    // generateChunkは時間がかかるため、ロックは最小限にするのが理想
    // ここでロックすると、チャンク生成中にm_chunksがロックされ、他のスレッドからのアクセスをブロックする可能性があります。
    // しかし、m_chunksへの書き込みはここでのみ発生するため、ここではロックが必要です。
    std::lock_guard<std::mutex> lock(m_chunksMutex); // m_chunksへの書き込みアクセスを保護
    for (const auto& coord : chunksToLoad) {
        std::cout << "DEBUG: loadChunksInArea - Generating new chunk: (" << coord.x << "," << coord.y << "," << coord.z << ")\n";
        std::shared_ptr<Chunk> newChunk = generateChunk(coord); // generateChunkは同期的に実行
        m_chunks[coord] = newChunk;
    }
    std::cout << "DEBUG: loadChunksInArea - Finished for center: (" << centerChunkCoord.x << "," << centerChunkCoord.y << "," << centerChunkCoord.z << ")\n";
}

// 不要なチャンクをアンロード（描画距離外に出たチャンク）
// 不要なチャンクをアンロード（描画距離外に出たチャンク）
void ChunkManager::unloadDistantChunks(const glm::ivec3 &centerChunkCoord)
{
    std::cout << "DEBUG: unloadDistantChunks - Start for center: (" << centerChunkCoord.x << "," << centerChunkCoord.y << "," << centerChunkCoord.z << ")\n";
    std::vector<glm::ivec3> chunksToUnload;
    {
        std::lock_guard<std::mutex> lock(m_chunksMutex); // m_chunksへの読み込みアクセスを保護
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
    } // m_chunksMutex ロック解放

    // アンロードリストにあるチャンクをマップから削除します
    // m_chunksとm_meshQueueMutexの両方にアクセスするので、std::lockを使用
    std::unique_lock<std::mutex> lock_chunks(m_chunksMutex, std::defer_lock); 
    std::unique_lock<std::mutex> lock_queue(m_meshQueueMutex, std::defer_lock);
    
    std::lock(lock_chunks, lock_queue); // 両方を安全にロック

    for (const auto &coord : chunksToUnload)
    {
        std::cout << "DEBUG: unloadDistantChunks - Unloading chunk: (" << coord.x << "," << coord.y << "," << coord.z << ")\n";
        m_chunkRenderData.erase(coord); 

        // m_chunksPendingMeshGeneration は既に m_meshQueueMutex で保護されている
        m_chunksPendingMeshGeneration.erase(coord);
        // m_generatedMeshQueue は pop するだけなので、ここでは変更しない

        m_chunks.erase(coord);          
    }
    std::cout << "DEBUG: unloadDistantChunks - Finished.\n";
}

// ワールド座標からチャンク座標を計算します
glm::ivec3 ChunkManager::getChunkCoordFromWorldPos(const glm::vec3 &worldPos) const
{
    // プレイヤーのワールド座標をチャンクのサイズで割って、どのチャンクにいるかを計算します
    // 負の座標の場合も正しく動作するように std::floor を使用しています
    int chunkX = static_cast<int>(std::floor(worldPos.x / m_chunkSize));
    int chunkY = static_cast<int>(std::floor(worldPos.y / m_chunkSize));
    int chunkZ = static_cast<int>(std::floor(worldPos.z / m_chunkSize));
    return glm::ivec3(chunkX, chunkY, chunkZ);
}