#ifndef CHUNK_MANAGER_HPP
#define CHUNK_MANAGER_HPP

#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>
#include "chunk/chunk.hpp"
#include "chunk_mesh_generator.hpp" // ChunkMeshData, ChunkRenderData のために必要
#include "chunk_renderer.hpp"     // ChunkMeshData, ChunkRenderData のために必要

// チャンクのワールド座標をキーとするハッシュ関数
struct Vec3iHash {
    size_t operator()(const glm::ivec3& v) const {
        return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1) ^ (std::hash<int>()(v.z) << 2);
    }
};

class ChunkManager {
public:
    ChunkManager(int chunkSize, float noiseScale, int renderDistance);
    ~ChunkManager();

    // プレイヤーの位置に基づいてチャンクを更新（ロード/アンロード）
    void update(const glm::vec3& playerPosition);

    // 指定されたワールド座標のチャンクが存在するかどうかをチェック
    bool hasChunk(const glm::ivec3& chunkCoord) const;

    // 指定されたチャンク座標のチャンクを取得
    std::shared_ptr<Chunk> getChunk(const glm::ivec3& chunkCoord);

    // レンダリングのためのすべてのチャンクのChunkRenderDataを取得
    const std::unordered_map<glm::ivec3, ChunkRenderData, Vec3iHash>& getAllRenderData() const {
        return m_chunkRenderData;
    }

private:
    int m_chunkSize;
    float m_noiseScale;
    int m_renderDistance; // プレイヤーからどれくらいの距離までチャンクをロードするか

    // キーはチャンクのワールド座標（例: (0,0,0), (1,0,0) など、チャンクごとの原点）
    std::unordered_map<glm::ivec3, std::shared_ptr<Chunk>, Vec3iHash> m_chunks;
    std::unordered_map<glm::ivec3, ChunkRenderData, Vec3iHash> m_chunkRenderData;

    // チャンクを生成して初期化
    std::shared_ptr<Chunk> generateChunk(const glm::ivec3& chunkCoord);

    // チャンクのメッシュを生成し、レンダリングデータを更新
    void updateChunkMesh(const glm::ivec3& chunkCoord, std::shared_ptr<Chunk> chunk);

    // 指定された座標範囲内のチャンクをロード
    void loadChunksInArea(const glm::ivec3& centerChunkCoord);

    // 不要なチャンクをアンロード
    void unloadDistantChunks(const glm::ivec3& centerChunkCoord);

    // ワールド座標からチャンク座標を計算
    glm::ivec3 getChunkCoordFromWorldPos(const glm::vec3& worldPos) const;
};

#endif // CHUNK_MANAGER_HPP