#ifndef CHUNK_PROCESSOR_HPP
#define CHUNK_PROCESSOR_HPP

#include <memory>
#include <glm/glm.hpp>
#include "chunk/chunk.hpp"
#include "chunk_mesh_generator.hpp"
#include "terrain_generator.hpp"

// NeighborChunkProvider インターフェースを定義
// チャンクプロセッサが隣接チャンクを取得するための抽象インターフェース
class NeighborChunkProvider {
public:
    virtual ~NeighborChunkProvider() = default;
    virtual std::shared_ptr<Chunk> getChunk(const glm::ivec3& chunkCoord) = 0;
};

class ChunkProcessor {
public:
    ChunkProcessor(int chunkSize, std::unique_ptr<TerrainGenerator> terrainGenerator);

    // チャンクのボクセルデータを生成する (非同期で実行される計算処理)
    std::shared_ptr<Chunk> generateChunkData(const glm::ivec3& chunkCoord);

    // チャンクのメッシュデータを生成する (非同期で実行される計算処理)
    // 隣接チャンクのデータを取得するために NeighborChunkProvider を使用
    ChunkMeshData generateMeshForChunk(const glm::ivec3& chunkCoord, std::shared_ptr<Chunk> chunk,
                                       NeighborChunkProvider* neighborProvider);

private:
    int m_chunkSize;
    std::unique_ptr<TerrainGenerator> m_terrainGenerator;

    // 隣接チャンク取得のためのヘルパー関数
    // ChunkProcessor 内部でのみ使用されるため、private に定義
    const Chunk* getNeighbor(const glm::ivec3& currentChunkCoord, const glm::ivec3& offset,
                             NeighborChunkProvider* neighborProvider);
};

#endif // CHUNK_PROCESSOR_HPP