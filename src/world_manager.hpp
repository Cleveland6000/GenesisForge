#ifndef WORLD_MANAGER_HPP
#define WORLD_MANAGER_HPP

#include <glm/glm.hpp>
#include <map>
#include <memory> // std::unique_ptr を使うため
#include "chunk/chunk.hpp"
#include "chunk_renderer.hpp" // ChunkRenderData の定義のため
#include "noise/perlin_noise_2d.hpp" // 地形生成のため

// チャンクのワールド座標（チャンクのインデックス）
// glm::ivec3 をキーとしてstd::mapで使用するためにハッシュ関数が必要な場合があるが、
// std::map はデフォルトで < を使うため、glm::ivec3 はそのままキーとして使用可能。

class WorldManager {
public:
    // コンストラクタ: チャンクサイズとレンダーディスタンス、ノイズ生成器を受け取る
    WorldManager(int chunkSize, int renderDistance, std::unique_ptr<PerlinNoise2D> perlinNoise);
    ~WorldManager();

    // プレイヤーの位置に基づいてチャンクを更新（生成・アンロード）
    void update(const glm::vec3& playerWorldPos);

    // すべてのアクティブなチャンクのレンダリングデータを取得
    const std::map<glm::ivec3, std::unique_ptr<ChunkRenderData>>& getRenderData() const;

    // チャンクが更新されたかどうかをチェックするフラグ
    bool hasChunkDataUpdated() const { return m_chunkDataUpdated; }
    void resetChunkDataUpdated() { m_chunkDataUpdated = false; }

private:
    int m_chunkSize;        // 各チャンクのボクセルサイズ
    int m_renderDistance;   // プレイヤー中心からチャンクをロードする半径（チャンク単位）

    std::unique_ptr<PerlinNoise2D> m_perlinNoise; // 地形生成用ノイズ

    // 現在ロードされているチャンクデータ (CPU側)
    std::map<glm::ivec3, std::unique_ptr<Chunk>> m_loadedChunks;
    // 現在GPUにアップロードされているレンダリングデータ
    std::map<glm::ivec3, std::unique_ptr<ChunkRenderData>> m_chunkRenderData;

    glm::ivec3 m_lastPlayerChunkCoord; // プレイヤーが最後にいたチャンクの座標

    bool m_chunkDataUpdated; // チャンクデータが変更されたかを示すフラグ

    // ヘルパー関数
    void loadChunk(const glm::ivec3& chunkCoord);
    void unloadChunk(const glm::ivec3& chunkCoord);
    void generateChunkVoxelData(Chunk& chunk, const glm::ivec3& chunkCoord);
    ChunkRenderData createRenderDataForChunk(const Chunk& chunk); // メッシュ生成とGPUアップロード

    // チャンクのワールド座標からピクセル単位のワールド座標を取得
    glm::vec3 getChunkWorldPosition(const glm::ivec3& chunkCoord) const;
};

#endif // WORLD_MANAGER_HPP