#include "voxel_accessor.hpp"

VoxelAccessor::VoxelAccessor(const Chunk& currentChunk,
                             const Chunk* neighbor_neg_x, const Chunk* neighbor_pos_x,
                             const Chunk* neighbor_neg_y, const Chunk* neighbor_pos_y,
                             const Chunk* neighbor_neg_z, const Chunk* neighbor_pos_z)
    : currentChunk_(currentChunk),
      neighbor_neg_x_(neighbor_neg_x), neighbor_pos_x_(neighbor_pos_x),
      neighbor_neg_y_(neighbor_neg_y), neighbor_pos_y_(neighbor_pos_y),
      neighbor_neg_z_(neighbor_neg_z), neighbor_pos_z_(neighbor_pos_z),
      chunkSize_(currentChunk.getSize())
{
}

size_t VoxelAccessor::getLocalVoxelIndex(int x, int y, int z) const
{
    // 範囲チェックは呼び出し元で行われるか、結果を使用する側で考慮されることを前提
    // ここでは単純なインデックス計算のみ
    return static_cast<size_t>(x + y * chunkSize_ + z * chunkSize_ * chunkSize_);
}

bool VoxelAccessor::isSolid(int x, int y, int z) const
{
    if (x >= 0 && x < chunkSize_ &&
        y >= 0 && y < chunkSize_ &&
        z >= 0 && z < chunkSize_)
    {
        // 現在のチャンク内のボクセル
        return currentChunk_.getVoxels()[getLocalVoxelIndex(x, y, z)];
    }
    else // 隣接チャンクのボクセル
    {
        const Chunk *targetChunk = nullptr;
        int targetX = x;
        int targetY = y;
        int targetZ = z;

        if (x < 0)
        {
            targetChunk = neighbor_neg_x_;
            targetX = chunkSize_ + x;
        }
        else if (x >= chunkSize_)
        {
            targetChunk = neighbor_pos_x_;
            targetX = x - chunkSize_;
        }
        else if (y < 0)
        {
            targetChunk = neighbor_neg_y_;
            targetY = chunkSize_ + y;
        }
        else if (y >= chunkSize_)
        {
            targetChunk = neighbor_pos_y_;
            targetY = y - chunkSize_;
        }
        else if (z < 0)
        {
            targetChunk = neighbor_neg_z_;
            targetZ = chunkSize_ + z;
        }
        else if (z >= chunkSize_)
        {
            targetChunk = neighbor_pos_z_;
            targetZ = z - chunkSize_;
        }

        if (targetChunk != nullptr &&
            targetX >= 0 && targetX < chunkSize_ &&
            targetY >= 0 && targetY < chunkSize_ &&
            targetZ >= 0 && targetZ < chunkSize_)
        {
            return targetChunk->getVoxels()[getLocalVoxelIndex(targetX, targetY, targetZ)];
        }
    }
    return false; // 範囲外またはチャンクが存在しない場合はソリッドではないとみなす
}