#include "world.hpp"
#include <iostream>
#include <set>
#include <chrono>

// コンストラクタ
// ここでマップを初期化する際に、カスタム比較関数を指定する必要はありません。
// std::mapのテンプレート引数で既に指定されているため、デフォルトコンストラクタが使用されます。
World::World(int chunkSize, int renderDistance, float cubeSpacing)
    : m_chunkSize(chunkSize), m_renderDistance(renderDistance), m_cubeSpacing(cubeSpacing),
      m_perlinNoise(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count()))
{
    std::cout << "World: Initialized with chunkSize=" << m_chunkSize
              << ", renderDistance=" << m_renderDistance
              << ", cubeSpacing=" << m_cubeSpacing << ".\n";
}

// デストラクタ
World::~World() {
    std::cout << "World: Cleaning up " << m_chunks.size() << " chunks.\n";
    // 全てのチャンクのレンダリングデータを解放します
    for (auto& pair : m_chunkRenderDataMap) {
        ChunkRenderer::deleteChunkRenderData(pair.second);
    }
    m_chunkRenderDataMap.clear();
    m_chunks.clear(); // unique_ptrが自動的にメモリを解放します
}

// チャンクのボクセルデータを生成します
void World::generateVoxelData(Chunk& chunk, int chunkX, int chunkZ) {
    float scale = 0.05f; // パーリンノイズのスケール
    // チャンクのワールド座標におけるオフセット
    float offsetX = static_cast<float>(chunkX * m_chunkSize);
    float offsetZ = static_cast<float>(chunkZ * m_chunkSize);

    for (int x = 0; x < m_chunkSize; ++x) {
        for (int z = 0; z < m_chunkSize; ++z) {
            // ワールド座標でノイズを計算します
            float worldX = offsetX + x;
            float worldZ = offsetZ + z;
            // ノイズ値に基づいて高さを決定します
            // (ノイズ + 1.0) / 2.0 で 0.0 から 1.0 の範囲に正規化し、それを最大高さにかけます
            int height = static_cast<int>((m_perlinNoise.noise(worldX * scale, worldZ * scale) + 1.0f) / 2.0f * (m_chunkSize / 2.0f)) + (m_chunkSize / 4); // ベースの高さを追加

            for (int y = 0; y < m_chunkSize; ++y) {
                // Y座標が高さより低い場合にボクセルを存在させます
                chunk.setVoxel(x, y, z, y <= height);
            }
        }
    }
    chunk.setDirty(true); // ボクセルデータが変更されたのでメッシュを再生成する必要があります
}

// チャンクのメッシュを生成または再生成します
void World::regenerateChunkMesh(const glm::ivec3& chunkCoord) {
    Chunk* chunk = getChunk(chunkCoord);
    if (!chunk) {
        std::cerr << "World: Cannot regenerate mesh for non-existent chunk at "
                  << chunkCoord.x << ", " << chunkCoord.y << ", " << chunkCoord.z << "\n";
        return;
    }

    // 既存のレンダリングデータがあれば解放します
    auto it = m_chunkRenderDataMap.find(chunkCoord);
    if (it != m_chunkRenderDataMap.end()) {
        ChunkRenderer::deleteChunkRenderData(it->second);
        m_chunkRenderDataMap.erase(it);
    }

    // 新しいメッシュデータを生成します
    ChunkMeshData meshData = ChunkMeshGenerator::generateMesh(*chunk, m_cubeSpacing);

    // メッシュデータが空でなければ、OpenGLレンダリングデータを生成して格納します
    if (!meshData.vertices.empty() && !meshData.indices.empty()) {
        m_chunkRenderDataMap[chunkCoord] = ChunkRenderer::createChunkRenderData(meshData);
        std::cout << "World: Mesh regenerated and render data created for chunk "
                  << chunkCoord.x << ", " << chunkCoord.y << ", " << chunkCoord.z << "\n";
    } else {
        std::cout << "World: No mesh data generated for chunk "
                  << chunkCoord.x << ", " << chunkCoord.y << ", " << chunkCoord.z << " (empty).\n";
    }
    chunk->setDirty(false); // メッシュが更新されたのでダーティフラグをクリアします
}

// チャンクをロードします
void World::loadChunk(const glm::ivec3& chunkCoord) {
    if (m_chunks.count(chunkCoord) > 0) {
        // すでにロードされています
        return;
    }
    std::cout << "World: Loading chunk " << chunkCoord.x << ", " << chunkCoord.y << ", " << chunkCoord.z << "\n";

    auto newChunk = std::make_unique<Chunk>(m_chunkSize);
    // ボクセルデータを生成します（チャンクのY座標を地形生成に含めます）
    generateVoxelData(*newChunk, chunkCoord.x, chunkCoord.z);

    m_chunks[chunkCoord] = std::move(newChunk);
    regenerateChunkMesh(chunkCoord); // メッシュを生成します
}

// チャンクをアンロードします
void World::unloadChunk(const glm::ivec3& chunkCoord) {
    if (m_chunks.count(chunkCoord) == 0) {
        // ロードされていません
        return;
    }
    std::cout << "World: Unloading chunk " << chunkCoord.x << ", " << chunkCoord.y << ", " << chunkCoord.z << "\n";

    // レンダリングデータがあれば解放します
    auto renderDataIt = m_chunkRenderDataMap.find(chunkCoord);
    if (renderDataIt != m_chunkRenderDataMap.end()) {
        ChunkRenderer::deleteChunkRenderData(renderDataIt->second);
        m_chunkRenderDataMap.erase(renderDataIt);
    }

    m_chunks.erase(chunkCoord); // チャンクオブジェクトを削除します
}

// プレイヤーの位置に基づいてチャンクを更新します
void World::updateChunks(const glm::vec3& playerPosition) {
    // プレイヤーのワールド座標から、現在のチャンクグリッド座標を計算します
    int playerChunkX = static_cast<int>(std::floor(playerPosition.x / (m_chunkSize * m_cubeSpacing)));
    int playerChunkY = static_cast<int>(std::floor(playerPosition.y / (m_chunkSize * m_cubeSpacing))); // Y軸も考慮するなら
    int playerChunkZ = static_cast<int>(std::floor(playerPosition.z / (m_chunkSize * m_cubeSpacing)));

    // std::setもカスタム比較関数を必要とします
    std::set<glm::ivec3, Ivector3Comparator> chunksToLoad;
    std::set<glm::ivec3, Ivector3Comparator> currentLoadedChunks;

    // 現在ロードされているチャンクのリストを構築します
    for (const auto& pair : m_chunks) {
        currentLoadedChunks.insert(pair.first);
    }

    // ロードすべきチャンクの集合を決定します
    for (int x = -m_renderDistance; x <= m_renderDistance; ++x) {
        // Y軸方向のチャンク管理は、ゲームの要件に応じて調整してください。
        // 単純な地形の場合、Y軸方向は1つで十分かもしれませんし、
        // 複数階層のチャンクが必要な場合はループを回す必要があります。
        // ここでは、プレイヤーがいるYチャンクを中心に、上下に1チャンクずつ探索する例としてYループを追加します。
        // 必要に応じて Y-1, Y, Y+1 のような範囲を検討してください。
        for (int y = -1; y <= 1; ++y) { // プレイヤーがいるYチャンクの上下1チャンクを探索
            for (int z = -m_renderDistance; z <= m_renderDistance; ++z) {
                glm::ivec3 targetChunkCoord(playerChunkX + x, playerChunkY + y, playerChunkZ + z);
                chunksToLoad.insert(targetChunkCoord);
            }
        }
    }

    // アンロードすべきチャンクを特定し、アンロードします
    for (const auto& chunkCoord : currentLoadedChunks) {
        if (chunksToLoad.find(chunkCoord) == chunksToLoad.end()) {
            unloadChunk(chunkCoord);
        }
    }

    // ロードすべきチャンクをロードします
    for (const auto& chunkCoord : chunksToLoad) {
        loadChunk(chunkCoord);
    }

    // ダーティなチャンクのメッシュを再生成します
    // ワールド更新ループ中にチャンクがダーティになる可能性があるため、ここでチェックして再生成します。
    // (例: チャンクのボクセルが編集された場合など)
    for (auto& pair : m_chunks) {
        if (pair.second->isDirty()) {
            regenerateChunkMesh(pair.first);
        }
    }
}

// 特定のチャンク座標にあるチャンクを取得します
Chunk* World::getChunk(const glm::ivec3& chunkCoord) {
    auto it = m_chunks.find(chunkCoord);
    if (it != m_chunks.end()) {
        return it->second.get();
    }
    return nullptr;
}
