#include "world_manager.hpp"
#include "chunk_mesh_generator.hpp" // メッシュ生成のため
#include <iostream> // デバッグ出力のため

WorldManager::WorldManager(int chunkSize, int renderDistance, std::unique_ptr<PerlinNoise2D> perlinNoise)
    : m_chunkSize(chunkSize),
      m_renderDistance(renderDistance),
      m_perlinNoise(std::move(perlinNoise)),
      m_lastPlayerChunkCoord(glm::ivec3(std::numeric_limits<int>::max())), // 初期値として到達不能な座標を設定
      m_chunkDataUpdated(false)
{
    // コンストラクタでは何もロードしない。updateでプレイヤー位置を元にロードを開始する。
}

WorldManager::~WorldManager() {
    // マップがunique_ptrを保持しているので、自動的に解放される
}

void WorldManager::update(const glm::vec3& playerWorldPos) {
    // プレイヤーの現在チャンク座標を計算
    // ボクセルサイズが1.0fなので、チャンクサイズで割って整数座標を得る
    glm::ivec3 currentPlayerChunkCoord = glm::ivec3(
        static_cast<int>(floor(playerWorldPos.x / m_chunkSize)),
        static_cast<int>(floor(playerWorldPos.y / m_chunkSize)),
        static_cast<int>(floor(playerWorldPos.z / m_chunkSize))
    );

    // プレイヤーが別のチャンクに移動した場合、チャンクのロード/アンロードを更新
    if (currentPlayerChunkCoord != m_lastPlayerChunkCoord) {
        std::cout << "Player moved to chunk: (" << currentPlayerChunkCoord.x << ", "
                  << currentPlayerChunkCoord.y << ", " << currentPlayerChunkCoord.z << ")\n";

        // 新しいチャンクをロード
        for (int x = -m_renderDistance; x <= m_renderDistance; ++x) {
            for (int y = -m_renderDistance; y <= m_renderDistance; ++y) {
                for (int z = -m_renderDistance; z <= m_renderDistance; ++z) {
                    glm::ivec3 chunkCoordToLoad = currentPlayerChunkCoord + glm::ivec3(x, y, z);
                    // まだロードされていないチャンクであればロード
                    if (m_loadedChunks.find(chunkCoordToLoad) == m_loadedChunks.end()) {
                        loadChunk(chunkCoordToLoad);
                        m_chunkDataUpdated = true; // 新しいデータが追加された
                    }
                }
            }
        }

        // 範囲外に出たチャンクをアンロード
        std::vector<glm::ivec3> chunksToUnload;
        for (const auto& pair : m_loadedChunks) {
            const glm::ivec3& loadedChunkCoord = pair.first;
            // プレイヤーからの距離をチェック
            // (x - currentX)^2 + (y - currentY)^2 + (z - currentZ)^2 <= renderDistance^2 のようにするべきだが、
            // 今回はシンプルなボックス範囲でチェック
            if (abs(loadedChunkCoord.x - currentPlayerChunkCoord.x) > m_renderDistance ||
                abs(loadedChunkCoord.y - currentPlayerChunkCoord.y) > m_renderDistance ||
                abs(loadedChunkCoord.z - currentPlayerChunkCoord.z) > m_renderDistance) {
                chunksToUnload.push_back(loadedChunkCoord);
            }
        }

        for (const auto& chunkCoord : chunksToUnload) {
            unloadChunk(chunkCoord);
            m_chunkDataUpdated = true; // データが削除された
        }

        m_lastPlayerChunkCoord = currentPlayerChunkCoord;
    }
}

const std::map<glm::ivec3, std::unique_ptr<ChunkRenderData>>& WorldManager::getRenderData() const {
    return m_chunkRenderData;
}

void WorldManager::loadChunk(const glm::ivec3& chunkCoord) {
    std::cout << "Loading chunk: (" << chunkCoord.x << ", " << chunkCoord.y << ", " << chunkCoord.z << ")\n";
    // チャンクデータ (CPU側) を生成
    std::unique_ptr<Chunk> newChunk = std::make_unique<Chunk>(m_chunkSize);
    generateChunkVoxelData(*newChunk, chunkCoord);
    m_loadedChunks[chunkCoord] = std::move(newChunk);

    // レンダリングデータ (GPU側) を生成
    ChunkRenderData renderData = createRenderDataForChunk(*m_loadedChunks[chunkCoord]);
    // モデル行列を設定 (チャンクのワールド位置にオフセット)
    renderData.modelMatrix = glm::translate(glm::mat4(1.0f), getChunkWorldPosition(chunkCoord));
    m_chunkRenderData[chunkCoord] = std::make_unique<ChunkRenderData>(std::move(renderData));
}

void WorldManager::unloadChunk(const glm::ivec3& chunkCoord) {
    std::cout << "Unloading chunk: (" << chunkCoord.x << ", " << chunkCoord.y << ", " << chunkCoord.z << ")\n";
    m_loadedChunks.erase(chunkCoord);
    m_chunkRenderData.erase(chunkCoord); // unique_ptrのデストラクタがVAO/VBO/EBOを解放する
}

void WorldManager::generateChunkVoxelData(Chunk& chunk, const glm::ivec3& chunkCoord) {
    float scale = 0.05f; // Perlin Noiseのスケール

    // チャンクのワールド位置オフセットを考慮してノイズを計算
    for (int x = 0; x < m_chunkSize; ++x) {
        for (int y = 0; y < m_chunkSize; ++y) {
            for (int z = 0; z < m_chunkSize; ++z) {
                // ワールド座標におけるボクセルの位置
                float worldX = (chunkCoord.x * m_chunkSize + x) * scale;
                float worldZ = (chunkCoord.z * m_chunkSize + z) * scale;
                float worldY = (chunkCoord.y * m_chunkSize + y); // Y軸はPerlin Noiseの高さオフセットに使う

                // 高さマップとしてPerlin Noiseを使用
                float height = (m_perlinNoise->noise(worldX, worldZ) * 6) + (m_chunkSize / 2.0f); // チャンクのYオフセットを考慮

                // ボクセルを設定 (ここではy座標がnoise高さ以下であればボクセルが存在)
                // 各チャンクのYオフセットを考慮してボクセルを生成
                chunk.setVoxel(x, y, z, (worldY <= height));
            }
        }
    }
}

ChunkRenderData WorldManager::createRenderDataForChunk(const Chunk& chunk) {
    // ChunkMeshGeneratorを使ってCPU側メッシュデータを生成
    ChunkMeshData meshData = ChunkMeshGenerator::generateMesh(chunk); // cubeSpacingはChunkMeshGeneratorでデフォルト1.0f

    // ChunkRendererを使ってGPU側レンダリングデータを生成
    return ChunkRenderer::createChunkRenderData(meshData);
}

glm::vec3 WorldManager::getChunkWorldPosition(const glm::ivec3& chunkCoord) const {
    return glm::vec3(
        static_cast<float>(chunkCoord.x * m_chunkSize),
        static_cast<float>(chunkCoord.y * m_chunkSize),
        static_cast<float>(chunkCoord.z * m_chunkSize)
    );
}