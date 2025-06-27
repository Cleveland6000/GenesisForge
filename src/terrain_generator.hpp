#ifndef TERRAIN_GENERATOR_HPP
#define TERRAIN_GENERATOR_HPP

#include <memory>
#include "noise/perlin_noise_2d.hpp" // PerlinNoise2D のために必要

class TerrainGenerator {
public:
    // コンストラクタにオクターブ関連のパラメータを追加
    TerrainGenerator(unsigned int noiseSeed, float noiseScale, int worldMaxHeight, int groundLevel,
                     int octaves, float lacunarity, float persistence);
    
    // 指定されたワールド座標のボクセルがソリッドであるかを判定
    bool isVoxelSolid(float worldX, float worldY, float worldZ) const;

private:
    std::unique_ptr<PerlinNoise2D> m_perlinNoise;
    float m_noiseScale;
    int m_worldMaxHeight;
    int m_groundLevel;

    // オクターブに関するパラメータ
    int m_octaves;      // オクターブの数
    float m_lacunarity; // 各オクターブの周波数が倍になる係数（通常2.0）
    float m_persistence; // 各オクターブの振幅が減衰する係数（通常0.5）
};

#endif // TERRAIN_GENERATOR_HPP
