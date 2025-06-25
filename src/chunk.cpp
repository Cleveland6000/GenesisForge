#include "chunk.hpp"
#include <stdexcept> // std::out_of_range のために必要

// コンストラクタの実装
Chunk::Chunk(int size, float density) // spacingを削除
    : m_size(size) // m_spacingの初期化を削除
{
    if (size <= 0) {
        throw std::invalid_argument("Chunk size must be positive.");
    }
    if (density < 0.0f || density > 1.0f) {
        throw std::invalid_argument("Voxel density must be between 0.0 and 1.0.");
    }

    m_voxels.resize(m_size * m_size * m_size);

    // ランダムなグリッドデータを生成
    std::random_device rd;  // シード生成
    std::mt19937 gen(rd()); // メルセンヌ・ツイスター法による乱数エンジン
    
    // 真になる確率を調整 (density)
    std::bernoulli_distribution dist(density); 

    for (int x = 0; x < m_size; ++x) {
        for (int y = 0; y < m_size; ++y) {
            for (int z = 0; z < m_size; ++z) {
                m_voxels[getIndex(x, y, z)] = dist(gen); // 乱数に基づいてtrue/falseを設定
            }
        }
    }
}

// 3D座標から1次元インデックスを計算するヘルパー関数
size_t Chunk::getIndex(int x, int y, int z) const {
    // 範囲チェック (デバッグ用やリリースビルドのassertに置き換えることも可能)
    if (x < 0 || x >= m_size ||
        y < 0 || y >= m_size ||
        z < 0 || z >= m_size) {
        throw std::out_of_range("Voxel coordinates out of chunk bounds.");
    }
    return static_cast<size_t>(x + y * m_size + z * m_size * m_size);
}

// 指定された座標のボクセルが存在するかどうかを返す
bool Chunk::getVoxel(int x, int y, int z) const {
    // getIndex内で範囲チェックが行われる
    return m_voxels[getIndex(x, y, z)];
}
