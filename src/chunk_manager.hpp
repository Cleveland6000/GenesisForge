#ifndef CHUNK_MANAGER_HPP
#define CHUNK_MANAGER_HPP

#include <glm/glm.hpp>
#include <map>
#include <memory>
#include <vector>
#include <queue>              // std::queue を追加
#include <mutex>              // std::mutex を追加
#include <condition_variable> // std::condition_variable を追加
#include <thread>             // std::thread を追加
#include <atomic>             // std::atomic を追加

#include "chunk/chunk.hpp"
#include "terrain_generator.hpp"
#include "chunk_renderer.hpp" // ChunkRenderData のために必要

// ChunkMeshData の前方宣言
struct ChunkMeshData;

// glm::ivec3 を std::map のキーとして使用するためのカスタム比較関数
// glm::ivec3 に operator< が定義されていないため
struct IVec3Compare {
    bool operator()(const glm::ivec3& a, const glm::ivec3& b) const {
        if (a.x != b.x) return a.x < b.x;
        if (a.y != b.y) return a.y < b.y;
        return a.z < b.z;
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
    std::shared_ptr<Chunk> getChunk(const glm::ivec3 &chunkCoord); // const を削除、またはオーバーロード

    // レンダリングデータの取得
    const std::map<glm::ivec3, ChunkRenderData, IVec3Compare>& getRenderData() const { return m_chunkRenderData; }

private:
    std::shared_ptr<Chunk> generateChunk(const glm::ivec3 &chunkCoord);
    void updateChunkMesh(const glm::ivec3 &chunkCoord, std::shared_ptr<Chunk> chunk); // これはワーカーに移行
    void loadChunksInArea(const glm::ivec3 &centerChunkCoord);
    void unloadDistantChunks(const glm::ivec3 &centerChunkCoord);
    glm::ivec3 getChunkCoordFromWorldPos(const glm::vec3 &worldPos) const;

    // ----- マルチスレッド化のための追加メンバー変数とメソッド -----
    void meshGenerationWorker(); // ワーカースレッドのエントリポイント
    void processCompletedMeshes(); // メインスレッドで完了したメッシュを処理

    std::vector<std::thread> m_workerThreads;
    std::queue<glm::ivec3> m_meshGenerationQueue;
    std::mutex m_meshGenerationMutex;
    std::condition_variable m_meshGenerationCondVar;
    std::atomic<bool> m_stopWorkers;

    std::queue<std::pair<glm::ivec3, ChunkMeshData>> m_completedMeshes;
    std::mutex m_completedMeshesMutex;
    std::condition_variable m_completedMeshesCondVar;

    // m_chunks と m_chunkRenderData のキーとしてカスタム比較を使用
    std::map<glm::ivec3, std::shared_ptr<Chunk>, IVec3Compare> m_chunks; // チャンクを保持するマップ
    mutable std::mutex m_chunksMutex; // m_chunks へのアクセスを保護するためのミューテックス (const メソッドでもロックできるように mutable に変更)
    // -------------------------------------------------------------

    int m_chunkSize;
    int m_renderDistance;
    std::unique_ptr<TerrainGenerator> m_terrainGenerator;
    glm::ivec3 m_lastPlayerChunkCoord;
    std::map<glm::ivec3, ChunkRenderData, IVec3Compare> m_chunkRenderData; // チャンクのレンダリングデータ
};

#endif // CHUNK_MANAGER_HPP