#ifndef CHUNK_HPP
#define CHUNK_HPP

#include <vector>
#include <random> // ランダム生成のために必要

class Chunk {
public:
    // コンストラクタ: グリッドサイズ、ボクセルが存在する確率を受け取る
    Chunk(int size, float density); // spacingを削除

    // 指定された座標のボクセルが存在するかどうかを返す
    bool getVoxel(int x, int y, int z) const;

    // グリッドの各次元のサイズを返す
    int getSize() const { return m_size; }

    // getSpacing() メソッドを削除

private:
    std::vector<bool> m_voxels; // ボクセルデータ (1次元配列で管理)
    int m_size;                 // グリッドの各次元のサイズ (例: 16)
    // float m_spacing;         // spacingを削除

    // 3D座標から1次元インデックスを計算するヘルパー関数
    size_t getIndex(int x, int y, int z) const;
};

#endif // CHUNK_HPP
