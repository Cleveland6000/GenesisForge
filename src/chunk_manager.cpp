#include "chunk_manager.hpp"
#include <iostream>
#include <chrono>                   // 時間計測に使用する場合は残す
#include <algorithm>                // std::abs のために必要
#include <limits>                   // std::numeric_limits のために必要
#include "chunk_mesh_generator.hpp" // ChunkMeshData, ChunkMeshGenerator::generateMesh のために必要

// コンストラクタ
ChunkManager::ChunkManager(int chunkSize, int renderDistanceXZ, unsigned int noiseSeed, float noiseScale,
                           int worldMaxHeight, int groundLevel, int octaves, float lacunarity, float persistence)
    : m_chunkSize(chunkSize), m_renderDistance(renderDistanceXZ),
      m_terrainGenerator(std::make_unique<TerrainGenerator>(noiseSeed, noiseScale, worldMaxHeight, groundLevel,
                                                            octaves, lacunarity, persistence)),
      m_lastPlayerChunkCoord(std::numeric_limits<int>::max()) // 初期値として到達不能な座標を設定
{
    // コンソール出力はデバッグ用です。リリース時には除去を検討してください。
    std::cout << "ChunkManager constructor called. ChunkSize: " << m_chunkSize
              << ", RenderDistanceXZ: " << m_renderDistance << std::endl;
}

// デストラクタ
ChunkManager::~ChunkManager()
{
    // コンソール出力はデバッグ用です。リリース時には除去を検討してください。
    std::cout << "ChunkManager destructor called." << std::endl;
    m_chunkRenderData.clear(); // 保持しているレンダリングデータをクリアします
}

// プレイヤーの位置に基づいてチャンクを更新（ロード/アンロード/メッシュ更新）
void ChunkManager::update(const glm::vec3 &playerPosition)
{
    // プレイヤーが現在いるチャンクの座標を計算します
    glm::ivec3 currentChunkCoord = getChunkCoordFromWorldPos(playerPosition);

    // プレイヤーのチャンク座標が変更された場合にのみ、不要なチャンクのアンロード処理を実行します
    // ロード処理は、プレイヤーが移動し続けているがチャンク境界をまたいでいない場合でも、
    // 新しいチャンクが視界に入ってくる可能性があるため、常に実行するのが安全です。
    if (currentChunkCoord != m_lastPlayerChunkCoord)
    {
        loadChunksInArea(currentChunkCoord);        // 指定された描画距離内のチャンクをロード（または既存を保持）
        unloadDistantChunks(currentChunkCoord);     // 描画距離外に出たチャンクをアンロード
        m_lastPlayerChunkCoord = currentChunkCoord; // 現在の座標を更新します

        for (auto &pair : m_chunks)
        {
            if (pair.second->isDirty())
            {
                updateChunkMesh(pair.first, pair.second); // メッシュを再生成
                pair.second->setDirty(false);             // ダーティフラグをリセットします
            }
        }
    }
    else
    {
        // チャンク座標が変わらない場合でもロードは確認します (hasChunkで重複は防がれます)
        // loadChunksInArea(currentChunkCoord);
    }

    // ダーティなチャンク（内容が変更されたチャンク）のメッシュを更新します
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

// チャンクのメッシュを生成し、レンダリングデータを更新します
void ChunkManager::updateChunkMesh(const glm::ivec3 &chunkCoord, std::shared_ptr<Chunk> chunk)
{
    // ChunkMeshGenerator を使用してメッシュデータを生成します
    ChunkMeshData meshData = ChunkMeshGenerator::generateMesh(*chunk);

    // レンダリングデータを更新します（既存のものを更新するか、新しく作成するロジックは ChunkRenderer 側で管理）
    // ここでは単純に置き換えですが、GPUリソースの再利用を検討すべき点です
    m_chunkRenderData[chunkCoord] = ChunkRenderer::createChunkRenderData(meshData);

    // コンソール出力はデバッグ用です
    // std::cout << "Updated mesh for chunk at: (" << chunkCoord.x << ", " << chunkCoord.y << ", " << chunkCoord.z << ")\n";
}

// 指定された座標範囲内のチャンクをロードします
void ChunkManager::loadChunksInArea(const glm::ivec3 &centerChunkCoord)
{
    // レンダリング距離に基づいてチャンクをロードする範囲を決定します
    for (int x = -m_renderDistance; x <= m_renderDistance; ++x)
    {
        for (int y = -m_renderDistance; y <= m_renderDistance; ++y)
        {
            for (int z = -m_renderDistance; z <= m_renderDistance; ++z)
            {
                // 現在のループ座標に対応するワールドチャンク座標です
                glm::ivec3 currentChunkCoord = glm::ivec3(centerChunkCoord.x + x, centerChunkCoord.y + y, centerChunkCoord.z + z);

                // もしまだそのチャンクがロードされていなければ
                if (!hasChunk(currentChunkCoord))
                {
                    // 新しくチャンクを生成してマップに追加します
                    std::shared_ptr<Chunk> newChunk = generateChunk(currentChunkCoord);
                    m_chunks[currentChunkCoord] = newChunk;
                    // generateChunk 内で newChunk->setDirty(true) が設定されるため、
                    // update メソッドのループでこのチャンクのメッシュが生成されます
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
        // 現在のチャンクと中心チャンクの距離を計算 (X, Y, Z 各軸)
        int dist_x = std::abs(coord.x - centerChunkCoord.x);
        int dist_y = std::abs(coord.y - centerChunkCoord.y);
        int dist_z = std::abs(coord.z - centerChunkCoord.z);

        // いずれかの軸で描画距離を超えている場合、アンロードリストに追加します
        if (dist_x > m_renderDistance || dist_y > m_renderDistance || dist_z > m_renderDistance)
        {
            chunksToUnload.push_back(coord);
        }
    }

    // アンロードリストにあるチャンクをマップから削除します
    for (const auto &coord : chunksToUnload)
    {
        // コンソール出力はデバッグ用です
        // std::cout << "Unloading chunk at: (" << coord.x << ", " << coord.y << ", " << coord.z << ")\n";
        m_chunkRenderData.erase(coord); // レンダリングデータも削除します
        m_chunks.erase(coord);          // チャンクデータも削除します
    }
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