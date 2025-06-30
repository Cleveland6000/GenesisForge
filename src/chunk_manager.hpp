#ifndef CHUNK_MANAGER_HPP
#define CHUNK_MANAGER_HPP

#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>
#include <future>
#include <vector>
#include "chunk/chunk.hpp"
#include "chunk_mesh_generator.hpp" // ChunkMeshData の定義のため
#include "chunk_renderer.hpp"
#include "terrain_generator.hpp" // ChunkProcessor のコンストラクタに渡すため
#include "chunk_processor.hpp" // 新しいクラスをインクルード

// チャンクのワールド座標をキーとするハッシュ関数 (変更なし)
struct Vec3iHash
{
    size_t operator()(const glm::ivec3 &v) const
    {
        return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1) ^ (std::hash<int>()(v.z) << 2);
    }
};

class ChunkManager : public NeighborChunkProvider // NeighborChunkProvider を実装
{
public:
    ChunkManager(int chunkSize, int renderDistanceXZ, unsigned int noiseSeed, float noiseScale,
                 int worldMaxHeight, int groundLevel, int octaves, float lacunarity, float persistence);
    ~ChunkManager();

    void update(const glm::vec3 &playerPosition);
    bool hasChunk(const glm::ivec3 &chunkCoord) const;
    std::shared_ptr<Chunk> getChunk(const glm::ivec3 &chunkCoord) override; // override を追加
    const std::unordered_map<glm::ivec3, ChunkRenderData, Vec3iHash> &getAllRenderData() const
    {
        return m_chunkRenderData;
    }

private:
    int m_chunkSize;
    int m_renderDistance;

    // TerrainGenerator は ChunkProcessor に移動
    std::unique_ptr<ChunkProcessor> m_chunkProcessor; // ChunkProcessor のインスタンスを持つ

    std::unordered_map<glm::ivec3, std::shared_ptr<Chunk>, Vec3iHash> m_chunks;
    std::unordered_map<glm::ivec3, ChunkRenderData, Vec3iHash> m_chunkRenderData;

    glm::ivec3 m_lastPlayerChunkCoord;

    // 非同期チャンク生成とメッシュ生成を管理するためのマップ
    std::unordered_map<glm::ivec3, std::future<std::shared_ptr<Chunk>>, Vec3iHash> m_pendingChunkGenerations;
    std::unordered_map<glm::ivec3, std::future<ChunkMeshData>, Vec3iHash> m_pendingMeshGenerations;

    // ヘルパーメソッド (変更なし)
    glm::ivec3 getChunkCoordFromWorldPos(const glm::vec3 &worldPos) const;
    void loadChunksInArea(const glm::ivec3 &centerChunkCoord);
    void unloadDistantChunks(const glm::ivec3 &centerChunkCoord);

    // OpenGLリソースの更新はメインスレッドで行うためのヘルパー (変更なし)
    void updateChunkRenderData(const glm::ivec3 &chunkCoord, const ChunkMeshData &meshData);
};

#endif // CHUNK_MANAGER_HPP