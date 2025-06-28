#ifndef CHUNK_MANAGER_HPP
#define CHUNK_MANAGER_HPP

#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>
#include "chunk/chunk.hpp"
#include "chunk_mesh_generator.hpp"
#include "chunk_renderer.hpp"
#include "terrain_generator.hpp"

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
    // コンストラクタにY軸方向の描画距離を追加
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
    int m_renderDistanceXZ; // X/Z軸方向の描画距離
    int m_renderDistanceY;  // ★★★ 新しく追加：Y軸方向の描画距離 ★★★

    std::unique_ptr<TerrainGenerator> m_terrainGenerator;

    std::unordered_map<glm::ivec3, std::shared_ptr<Chunk>, Vec3iHash> m_chunks;
    std::unordered_map<glm::ivec3, ChunkRenderData, Vec3iHash> m_chunkRenderData;

    std::shared_ptr<Chunk> generateChunk(const glm::ivec3 &chunkCoord);
    void updateChunkMesh(const glm::ivec3 &chunkCoord, std::shared_ptr<Chunk> chunk);
    void loadChunksInArea(const glm::ivec3 &centerChunkCoord);
    void unloadDistantChunks(const glm::ivec3 &centerChunkCoord);
    glm::ivec3 getChunkCoordFromWorldPos(const glm::vec3 &worldPos) const;
};

#endif // CHUNK_MANAGER_HPP
