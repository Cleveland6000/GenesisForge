#ifndef TERRAIN_GENERATOR_HPP
#define TERRAIN_GENERATOR_HPP

#include <memory>
#include "noise/perlin_noise_2d.hpp"

class TerrainGenerator {
public:
    TerrainGenerator(unsigned int noiseSeed, float noiseScale, int worldMaxHeight, int groundLevel,
                     int octaves, float lacunarity, float persistence);
    
    int getTerrainHeight(float worldX, float worldZ) const;

    // isVoxelSolid はもはや外部から直接呼び出す必要がないかもしれませんが、
    // 将来的な複雑なボクセルタイプ判定のために残しておくこともできます。
    // 今回の最適化では、ChunkManagerがgetTerrainHeightの結果とgroundLevelを使って直接判定するため、
    // このメソッドは使用されなくなります。
    bool isVoxelSolid(float worldX, float worldY, float worldZ) const; 

    // ChunkManager から groundLevel にアクセスするためのゲッター
    int getGroundLevel() const { return m_groundLevel; }

private:
    std::unique_ptr<PerlinNoise2D> m_perlinNoise;
    float m_noiseScale;
    int m_worldMaxHeight;
    int m_groundLevel;

    int m_octaves;
    float m_lacunarity;
    float m_persistence;
};

#endif // TERRAIN_GENERATOR_HPP
