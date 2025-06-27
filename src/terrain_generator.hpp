#ifndef TERRAIN_GENERATOR_HPP
#define TERRAIN_GENERATOR_HPP

#include <memory>
#include "noise/perlin_noise_2d.hpp" // PerlinNoise2D のために必要

class TerrainGenerator {
public:
    TerrainGenerator(unsigned int noiseSeed, float noiseScale, int worldMaxHeight, int groundLevel);
    
    // 指定されたワールド座標のボクセルがソリッドであるかを判定
    bool isVoxelSolid(float worldX, float floatY, float worldZ) const;

private:
    std::unique_ptr<PerlinNoise2D> m_perlinNoise;
    float m_noiseScale;
    int m_worldMaxHeight;
    int m_groundLevel;
};

#endif // TERRAIN_GENERATOR_HPP