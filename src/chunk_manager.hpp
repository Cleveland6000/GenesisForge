#ifndef CHUNK_MANAGER_HPP
#define CHUNK_MANAGER_HPP

#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>
#include <future> // std::future のために追加
#include <vector> // std::vector のために追加
#include "chunk/chunk.hpp"
#include "chunk_mesh_generator.hpp"
#include "chunk_renderer.hpp"
#include "terrain_generator.hpp"

// チャンクのワールド座標をキーとするハッシュ関数
struct Vec3iHash
{
    size_t operator()(const glm::ivec3 &v) const
    {
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

    glm::ivec3 m_lastPlayerChunkCoord; // 最後にプレイヤーがいたチャンク座標

    // 非同期チャンク生成とメッシュ生成を管理するためのマップ
    std::unordered_map<glm::ivec3, std::future<std::shared_ptr<Chunk>>, Vec3iHash> m_pendingChunkGenerations;
    std::unordered_map<glm::ivec3, std::future<ChunkMeshData>, Vec3iHash> m_pendingMeshGenerations;

    // ヘルパーメソッド
    glm::ivec3 getChunkCoordFromWorldPos(const glm::vec3 &worldPos) const;
    void loadChunksInArea(const glm::ivec3 &centerChunkCoord);
    void unloadDistantChunks(const glm::ivec3 &centerChunkCoord);

    // チャンク生成ロジックを分離 (非同期で実行される関数)
    std::shared_ptr<Chunk> generateChunkData(const glm::ivec3 &chunkCoord);

    // チャンクメッシュ生成ロジックを分離 (非同期で実行される関数)
    // メッシュデータのみを生成し、OpenGLリソースは扱わない
    // 隣接チャンクのデータを直接shared_ptrで受け取るように変更
    ChunkMeshData generateMeshForChunk(const glm::ivec3 &chunkCoord, std::shared_ptr<Chunk> chunk,
                                       std::shared_ptr<Chunk> neighbor_neg_x, std::shared_ptr<Chunk> neighbor_pos_x,
                                       std::shared_ptr<Chunk> neighbor_neg_y, std::shared_ptr<Chunk> neighbor_pos_y,
                                       std::shared_ptr<Chunk> neighbor_neg_z, std::shared_ptr<Chunk> neighbor_pos_z);

    // OpenGLリソースの更新はメインスレッドで行うためのヘルパー
    void updateChunkRenderData(const glm::ivec3 &chunkCoord, const ChunkMeshData &meshData);
};

#endif // CHUNK_MANAGER_HPP