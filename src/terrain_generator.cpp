#include "terrain_generator.hpp"
#include <iostream>
#include <algorithm> // std::max のために必要

// コンストラクタ: オクターブ関連のパラメータを受け取る
TerrainGenerator::TerrainGenerator(unsigned int noiseSeed, float noiseScale, int worldMaxHeight, int groundLevel,
                                 int octaves, float lacunarity, float persistence)
    : m_perlinNoise(std::make_unique<PerlinNoise2D>(noiseSeed)),
      m_noiseScale(noiseScale),
      m_worldMaxHeight(worldMaxHeight),
      m_groundLevel(groundLevel),
      m_octaves(octaves),
      m_lacunarity(lacunarity),
      m_persistence(persistence)
{
    if (!m_perlinNoise) {
        std::cerr << "Error: Failed to create PerlinNoise2D instance in TerrainGenerator.\n";
    }
    std::cout << "TerrainGenerator initialized. NoiseScale: " << m_noiseScale 
              << ", WorldMaxHeight: " << m_worldMaxHeight 
              << ", GroundLevel: " << m_groundLevel
              << ", Octaves: " << m_octaves
              << ", Lacunarity: " << m_lacunarity
              << ", Persistence: " << m_persistence << std::endl;
}

bool TerrainGenerator::isVoxelSolid(float worldX, float worldY, float worldZ) const {
    if (!m_perlinNoise) {
        return false; 
    }

    double totalNoise = 0.0;
    double maxAmplitude = 0.0; // 正規化のための最大振幅の合計
    double currentAmplitude = 1.0;
    double currentFrequency = 1.0;

    // 複数のオクターブを組み合わせてノイズを計算
    for (int i = 0; i < m_octaves; ++i) {
        // 現在の周波数と振幅でノイズを計算し、合計に加える
        totalNoise += m_perlinNoise->noise(worldX * m_noiseScale * currentFrequency, worldZ * m_noiseScale * currentFrequency) * currentAmplitude;
        
        // 最大振幅の合計を更新（正規化のため）
        maxAmplitude += currentAmplitude;
        
        // 次のオクターブのために振幅と周波数を更新
        currentAmplitude *= m_persistence;
        currentFrequency *= m_lacunarity;
    }

    // ノイズ値を正規化する (範囲を -1.0 から 1.0 に近づける)
    // maxAmplitudeが0になることを防ぐ
    double normalizedNoise = (maxAmplitude > 0.0) ? (totalNoise / maxAmplitude) : 0.0;
    
    // ノイズ結果を [0, 1] に正規化し、WORLD_MAX_HEIGHT にスケーリングして地形の高さを計算
    int terrainHeight = static_cast<int>((normalizedNoise + 1.0) * 0.5 * m_worldMaxHeight);

    // ボクセルがソリッドであるかどうかの判定
    // ワールドY座標が地形の高さ以下であればソリッド
    bool isSolid = (worldY < terrainHeight);
    
    // 地表レベルより下は常にソリッドにする、といった追加ルールも可能（コメントアウトされている例）
    // 例: もし地面より下なら常にソリッドにしたい場合
    // if (worldY < m_groundLevel) {
    //     isSolid = true;
    // } else {
    //     isSolid = (worldY < terrainHeight);
    // }

    return isSolid;
}
