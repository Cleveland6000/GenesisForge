#ifndef CHUNK_MANAGER_HPP
#define CHUNK_MANAGER_HPP

#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>
#include <future>
#include <vector>
#include <mutex> // std::mutex のために追加
#include "chunk/chunk.hpp"
#include "chunk_mesh_generator.hpp"
#include "chunk_renderer.hpp"
#include "terrain_generator.hpp"
#include "chunk_processor.hpp"
#include "thread_pool.hpp" // 新しい ThreadPool をインクルード

// チャンクのワールド座標をキーとするハッシュ関数 (変更なし)
struct Vec3iHash
{
    size_t operator()(const glm::ivec3 &v) const
    {
        return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1) ^ (std::hash<int>()(v.z) << 2);
    }
};

class ChunkManager : public NeighborChunkProvider
{
public:
    ChunkManager(int chunkSize, int renderDistanceXZ, unsigned int noiseSeed, float noiseScale,
                 int worldMaxHeight, int groundLevel, int octaves, float lacunarity, float persistence,
                 size_t numWorkerThreads);
    ~ChunkManager();

    void update(const glm::vec3 &playerPosition);
    bool hasChunk(const glm::ivec3 &chunkCoord) const; // ★ここがエラーの発生箇所
    std::shared_ptr<Chunk> getChunk(const glm::ivec3 &chunkCoord) override; // NeighborChunkProvider のオーバーライド
    const std::unordered_map<glm::ivec3, ChunkRenderData, Vec3iHash> &getAllRenderData() const
    {
        return m_chunkRenderData;
    }

private:
    int m_chunkSize;
    int m_renderDistance;

    std::unique_ptr<ChunkProcessor> m_chunkProcessor;
    std::unique_ptr<ThreadPool> m_threadPool;

    std::unordered_map<glm::ivec3, std::shared_ptr<Chunk>, Vec3iHash> m_chunks;
    std::unordered_map<glm::ivec3, ChunkRenderData, Vec3iHash> m_chunkRenderData;

    glm::ivec3 m_lastPlayerChunkCoord;

    // 非同期チャンク生成とメッシュ生成を管理するためのマップ
    std::unordered_map<glm::ivec3, std::future<std::shared_ptr<Chunk>>, Vec3iHash> m_pendingChunkGenerations;
    std::unordered_map<glm::ivec3, std::future<ChunkMeshData>, Vec3iHash> m_pendingMeshGenerations;

    // ★以下のミューテックスに 'mutable' キーワードを追加
    mutable std::mutex m_chunkMutex; // チャンクデータへのアクセスを保護するミューテックス
    mutable std::mutex m_pendingGenMutex; // m_pendingChunkGenerations と m_pendingMeshGenerations へのアクセスを保護するミューテックス

    glm::ivec3 getChunkCoordFromWorldPos(const glm::vec3 &worldPos) const;
    void loadChunksInArea(const glm::ivec3 &centerChunkCoord);
    void unloadDistantChunks(const glm::ivec3 &centerChunkCoord);

    void updateChunkRenderData(const glm::ivec3 &chunkCoord, const ChunkMeshData &meshData);
};

#endif // CHUNK_MANAGER_HPP