#include "terrain_generator.hpp"
#include <iostream>

TerrainGenerator::TerrainGenerator(unsigned int noiseSeed, float noiseScale, int worldMaxHeight, int groundLevel)
    : m_perlinNoise(std::make_unique<PerlinNoise2D>(noiseSeed)),
      m_noiseScale(noiseScale),
      m_worldMaxHeight(worldMaxHeight),
      m_groundLevel(groundLevel)
{
    if (!m_perlinNoise) {
        std::cerr << "Error: Failed to create PerlinNoise2D instance in TerrainGenerator.\n";
    }
    std::cout << "TerrainGenerator initialized. NoiseScale: " << m_noiseScale 
              << ", WorldMaxHeight: " << m_worldMaxHeight 
              << ", GroundLevel: " << m_groundLevel << std::endl;
}

bool TerrainGenerator::isVoxelSolid(float worldX, float worldY, float worldZ) const {
    if (!m_perlinNoise) {
        // ノイズジェネレータが初期化されていない場合は、デフォルトで非ソリッドとするか、エラーを報告
        return false; 
    }

    // Perlinノイズの値を生成 (入力は worldX * scale, worldZ * scale)
    double noiseValue = m_perlinNoise->noise(worldX * m_noiseScale, worldZ * m_noiseScale);
    
    // ノイズ結果を [0, 1] に正規化し、WORLD_MAX_HEIGHT にスケーリングして地形の高さを計算
    int terrainHeight = static_cast<int>((noiseValue + 1.0) * 0.5 * m_worldMaxHeight);

    // ボクセルがソリッドであるかどうかの判定
    // ワールドY座標が地形の高さ以下であればソリッド
    bool isSolid = (worldY < terrainHeight);

    // 例: もし地面より下なら常にソリッドにしたい場合
    // if (worldY < m_groundLevel) {
    //     isSolid = true;
    // } else {
    //     isSolid = (worldY < terrainHeight);
    // }

    return isSolid;
}