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

// ワールドX, Z座標における地形の高さを返す新しいメソッド
int TerrainGenerator::getTerrainHeight(float worldX, float worldZ) const {
    if (!m_perlinNoise) {
        return 0; // または適切なデフォルト値
    }

    double totalNoise = 0.0;
    double maxAmplitude = 0.0;
    double currentAmplitude = 1.0;
    double currentFrequency = 1.0;

    // 複数のオクターブを組み合わせてノイズを計算
    for (int i = 0; i < m_octaves; ++i) {
        totalNoise += m_perlinNoise->noise(worldX * m_noiseScale * currentFrequency, worldZ * m_noiseScale * currentFrequency) * currentAmplitude;
        maxAmplitude += currentAmplitude;
        currentAmplitude *= m_persistence;
        currentFrequency *= m_lacunarity;
    }

    double normalizedNoise = (maxAmplitude > 0.0) ? (totalNoise / maxAmplitude) : 0.0;
    
    // ノイズ結果を [0, WORLD_MAX_HEIGHT] の範囲にスケーリング
    // ここではノイズを純粋な地形の隆起として扱い、最終的な高さは GROUND_LEVEL をベースにする
    int terrainHeightOffset = static_cast<int>((normalizedNoise + 1.0) * 0.5 * (m_worldMaxHeight - m_groundLevel));
    return m_groundLevel + terrainHeightOffset;
}

// 指定されたワールド座標のボクセルがソリッドであるかを判定
// このメソッドは、TerrainGenerator::getTerrainHeight の結果を利用するように変更されます。
// また、GROUND_LEVEL 以下の判定は ChunkManager 側で行うため、ここでは純粋に地形の高さに基づく判定のみを行う。
bool TerrainGenerator::isVoxelSolid(float worldX, float worldY, float worldZ) const {
    int terrainHeight = getTerrainHeight(worldX, worldZ);
    return (worldY < terrainHeight);
    // GROUND_LEVEL 以下の判定は ChunkManager::generateChunk で行われます
}
