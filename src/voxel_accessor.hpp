#ifndef VOXEL_ACCESSOR_HPP
#define VOXEL_ACCESSOR_HPP

#include "chunk/chunk.hpp"
#include <vector>
#include <limits> // std::numeric_limits のために追加

// 隣接チャンクのオフセットをVoxelAccessorの内部に移動、あるいは引き続きグローバルで管理することも可能
// ここではChunkMeshGeneratorが使用するため、neighborOffsetsはChunkMeshGenerator.hppに残します。

class VoxelAccessor
{
public:
    VoxelAccessor(const Chunk& currentChunk,
                  const Chunk* neighbor_neg_x, const Chunk* neighbor_pos_x,
                  const Chunk* neighbor_neg_y, const Chunk* neighbor_pos_y,
                  const Chunk* neighbor_neg_z, const Chunk* neighbor_pos_z);

    // 指定されたワールド座標でのボクセルがソリッドかどうかを判断
    bool isSolid(int x, int y, int z) const;

private:
    const Chunk& currentChunk_;
    const Chunk* neighbor_neg_x_;
    const Chunk* neighbor_pos_x_;
    const Chunk* neighbor_neg_y_;
    const Chunk* neighbor_pos_y_;
    const Chunk* neighbor_neg_z_;
    const Chunk* neighbor_pos_z_;
    int chunkSize_;

    // チャンク内のローカルボクセルインデックスを取得
    size_t getLocalVoxelIndex(int x, int y, int z) const;
};

#endif // VOXEL_ACCESSOR_HPP