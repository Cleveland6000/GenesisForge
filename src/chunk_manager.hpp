#ifndef CHUNK_MANAGER_HPP
#define CHUNK_MANAGER_HPP

#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>
#include <future> // ★追加: std::future と std::future_status のために必要
#include <queue>  // ★追加: キューのために必要
#include <mutex>  // ★追加: ミューテックスのために必要

#include "chunk/chunk.hpp"
#include "chunk_mesh_generator.hpp"
#include "chunk_renderer.hpp"
#include "terrain_generator.hpp"
#include "thread_pool.hpp" // ★追加: ThreadPool クラスのために必要

// チャンクのワールド座標をキーとするハッシュ関数
struct Vec3iHash
{
    size_t operator()(const glm::ivec3 &v) const
    {
        // Y成分もハッシュに含めるように更新
        return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1) ^ (std::hash<int>()(v.z) << 2);
    }
};

class ChunkManager
{
public:
    ChunkManager(int chunkSize, int renderDistanceXZ, unsigned int noiseSeed, float noiseScale,
                 int worldMaxHeight, int groundLevel, int octaves, float lacunarity, float persistence);
    ~ChunkManager();

    void update(const glm::vec3 &playerPosition);
    bool hasChunk(const glm::ivec3 &chunkCoord) const;
    std::shared_ptr<Chunk> getChunk(const glm::ivec3 &chunkCoord);
    const std::unordered_map<glm::ivec3, ChunkRenderData, Vec3iHash> &getAllRenderData() const
    {
        return m_chunkRenderData;
    }

private:
    int m_chunkSize;
    int m_renderDistance;

    std::unique_ptr<TerrainGenerator> m_terrainGenerator;
    std::unordered_map<glm::ivec3, std::shared_ptr<Chunk>, Vec3iHash> m_chunks;
    std::unordered_map<glm::ivec3, ChunkRenderData, Vec3iHash> m_chunkRenderData;

    std::shared_ptr<Chunk> generateChunk(const glm::ivec3 &chunkCoord);
    // ★変更: 戻り値の型を ChunkMeshData に変更
    ChunkMeshData updateChunkMesh(const glm::ivec3 &chunkCoord, std::shared_ptr<Chunk> chunk);
    void loadChunksInArea(const glm::ivec3 &centerChunkCoord);
    void unloadDistantChunks(const glm::ivec3 &centerChunkCoord);
    glm::ivec3 getChunkCoordFromWorldPos(const glm::vec3 &worldPos) const;
    glm::ivec3 m_lastPlayerChunkCoord; // 前回のプレイヤーのチャンク座標

    // ★ここから追加
    std::unique_ptr<ThreadPool> m_threadPool; // スレッドプール

    // チャンク生成タスクを追跡するためのマップ
    std::unordered_map<glm::ivec3, std::future<std::shared_ptr<Chunk>>, Vec3iHash> m_pendingTerrainTasks;
    // メッシュ生成タスクを追跡するためのマップ
    std::unordered_map<glm::ivec3, std::future<ChunkMeshData>, Vec3iHash> m_pendingMeshTasks;
    // ★ここまで追加

    // 以前に提案したキューとミューテックスは、直接 future をポーリングする方式に変更したため、
    // 現在のコードでは使用していませんが、今後の拡張のために残しておくのは良いかもしれません。
    // std::queue<std::pair<glm::ivec3, std::shared_ptr<Chunk>>> m_generatedTerrainChunks;
    // std::queue<std::pair<glm::ivec3, ChunkMeshData>> m_generatedMeshes;
    // std::mutex m_chunkGenerationMutex;
    // std::mutex m_meshGenerationMutex;
};

#endif // CHUNK_MANAGER_HPP