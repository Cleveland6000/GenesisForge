#ifndef CHUNK_MANAGER_HPP
#define CHUNK_MANAGER_HPP

#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>
#include "chunk/chunk.hpp"
#include "chunk_mesh_generator.hpp"
#include "chunk_renderer.hpp"
// #include "noise/perlin_noise_2d.hpp" // TerrainGenerator が持つため不要

#include "terrain_generator.hpp" // 新しく追加

// チャンクのワールド座標をキーとするハッシュ関数
struct Vec3iHash {
    size_t operator()(const glm::ivec3& v) const {
        return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1) ^ (std::hash<int>()(v.z) << 2);
    }
};

class ChunkManager {
public:
    // コンストラクタからノイズ関連の引数を TerrainGenerator に渡すためのものに変更
    ChunkManager(int chunkSize, int renderDistance, unsigned int noiseSeed, float noiseScale, int worldMaxHeight, int groundLevel);
    ~ChunkManager();

    void update(const glm::vec3& playerPosition);
    bool hasChunk(const glm::ivec3& chunkCoord) const;
    std::shared_ptr<Chunk> getChunk(const glm::ivec3& chunkCoord);
    const std::unordered_map<glm::ivec3, ChunkRenderData, Vec3iHash>& getAllRenderData() const {
        return m_chunkRenderData;
    }

private:
    int m_chunkSize;
    int m_renderDistance;
    // m_worldMaxHeight, m_groundLevel, m_noiseScale は TerrainGenerator が管理するため削除
    // std::unique_ptr<PerlinNoise2D> m_perlinNoise; // TerrainGenerator が持つため削除

    std::unique_ptr<TerrainGenerator> m_terrainGenerator; // TerrainGenerator のインスタンスを追加

    std::unordered_map<glm::ivec3, std::shared_ptr<Chunk>, Vec3iHash> m_chunks;
    std::unordered_map<glm::ivec3, ChunkRenderData, Vec3iHash> m_chunkRenderData;

    std::shared_ptr<Chunk> generateChunk(const glm::ivec3& chunkCoord);
    void updateChunkMesh(const glm::ivec3& chunkCoord, std::shared_ptr<Chunk> chunk);
    void loadChunksInArea(const glm::ivec3& centerChunkCoord);
    void unloadDistantChunks(const glm::ivec3& centerChunkCoord);
    glm::ivec3 getChunkCoordFromWorldPos(const glm::vec3& worldPos) const;
};

#endif // CHUNK_MANAGER_HPP
