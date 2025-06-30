#ifndef CHUNK_MESH_GENERATOR_HPP
#define CHUNK_MESH_GENERATOR_HPP

#include <vector>
#include <cstdint>
#include <array>
#include <glm/glm.hpp>
#include <limits> // std::numeric_limits のために追加

#include "chunk/chunk.hpp" // Chunkクラスの定義のため

struct Vertex
{
    float x, y, z;
    float r, g, b;
    float u, v;
    float nx, ny, nz;
    float ao; // AO値を格納する新しい属性
};

struct ChunkMeshData
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

// 隣接チャンクのオフセットをグローバルに定義 (変更なし)
const std::array<glm::ivec3, 6> neighborOffsets = {
    glm::ivec3(0, 0, -1), // Back face (Z-)
    glm::ivec3(0, 0, 1),  // Front face (Z+)
    glm::ivec3(-1, 0, 0), // Left face (X-)
    glm::ivec3(1, 0, 0),  // Right face (X+)
    glm::ivec3(0, -1, 0), // Bottom face (Y-)
    glm::ivec3(0, 1, 0)   // Top face (Y+)
};

class ChunkMeshGenerator
{
public:
    static ChunkMeshData generateMesh(const Chunk &chunk,
                                      const Chunk* neighbor_neg_x = nullptr,
                                      const Chunk* neighbor_pos_x = nullptr,
                                      const Chunk* neighbor_neg_y = nullptr,
                                      const Chunk* neighbor_pos_y = nullptr,
                                      const Chunk* neighbor_neg_z = nullptr,
                                      const Chunk* neighbor_pos_z = nullptr
                                     );

private: // ヘルパー関数はプライベートに
    // ボクセルがソリッドかどうかを判断するヘルパー関数
    static bool isVoxelSolid(int x, int y, int z, int chunkSize,
                             const Chunk &currentChunk,
                             const Chunk *neighbor_neg_x, const Chunk *neighbor_pos_x,
                             const Chunk *neighbor_neg_y, const Chunk *neighbor_pos_y,
                             const Chunk *neighbor_neg_z, const Chunk *neighbor_pos_z);

    // アンビエントオクルージョン値を計算するヘルパー関数
    static float calculateAmbientOcclusion(int x, int y, int z, int chunkSize,
                                         const Chunk &currentChunk,
                                         const Chunk *neighbor_neg_x, const Chunk *neighbor_pos_x,
                                         const Chunk *neighbor_neg_y, const Chunk *neighbor_pos_y,
                                         const Chunk *neighbor_neg_z, const Chunk *neighbor_pos_z,
                                         float cornerDX, float cornerDY, float cornerDZ,
                                         int faceIndex);

    // UV座標を回転・反転させるヘルパー関数
    static glm::vec2 transformUV(const glm::vec2& uv, int rotationAmount, bool flipHorizontal);

    // 面のデータをmeshDataに追加するヘルパー関数
    static void addFaceToMeshData(ChunkMeshData& meshData, int x, int y, int z, int faceIndex,
                                  int rotationAmount, bool flipHorizontal, int chunkSize,
                                  const Chunk &currentChunk,
                                  const Chunk *neighbor_neg_x, const Chunk *neighbor_pos_x,
                                  const Chunk *neighbor_neg_y, const Chunk *neighbor_pos_y,
                                  const Chunk *neighbor_neg_z, const Chunk *neighbor_pos_z);

    // ボクセルインデックスを計算するヘルパー関数
    // 定義を .hpp ファイルに直接移動
    static inline size_t getVoxelIndex(int x, int y, int z, int chunkSize) {
        if (x < 0 || x >= chunkSize || y < 0 || y >= chunkSize || z < 0 || z >= chunkSize)
        {
            // 範囲外のアクセスは安全な値を返す
            return std::numeric_limits<size_t>::max(); // 無効なインデックス
        }
        return static_cast<size_t>(x + y * chunkSize + z * chunkSize * chunkSize);
    }

    // ボクセルから値を取得するヘルパー（getVoxelIndexをラップ）
    // 定義を .hpp ファイルに直接移動
    static inline bool getVoxelValue(int x, int y, int z, int chunkSize, const std::vector<bool>& voxels) {
        size_t index = getVoxelIndex(x, y, z, chunkSize);
        return (index != std::numeric_limits<size_t>::max()) ? voxels[index] : false;
    }
};

#endif // CHUNK_MESH_GENERATOR_HPP