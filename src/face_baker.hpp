#ifndef FACE_BAKER_HPP
#define FACE_BAKER_HPP

#include "mesh_types.hpp"   // 新しく追加
#include "voxel_accessor.hpp" // VoxelAccessor を使用
#include <glm/glm.hpp>
#include <array>
#include <vector> // std::vector のために追加

// 既存の定数。これらの定義はface_baker.cppにあり、ここではextern宣言として機能します。
// ただし、Vertexが定義された後でなければならない。
extern const std::vector<Vertex> baseCubeVertices;
extern const std::array<std::array<unsigned int, 4>, 6> cubeFaceBaseIndices;
extern const std::array<glm::vec3, 6> faceNormals;
extern const std::array<glm::vec2, 4> faceUVs;

class FaceBaker
{
public:
    explicit FaceBaker(const VoxelAccessor& accessor, int chunkSize);

    // 単一の面を生成し、頂点データをmeshDataに追加
    void bakeFace(ChunkMeshData& meshData, int x, int y, int z, int faceIndex,
                  int rotationAmount, bool flipHorizontal);

private:
    const VoxelAccessor& voxelAccessor_;
    int chunkSize_;

    // アンビエントオクルージョン値を計算
    float calculateAmbientOcclusion(int x, int y, int z,
                                    float cornerDX, float cornerDY, float cornerDZ,
                                    int faceIndex) const;

    // UV座標を回転・反転
    glm::vec2 transformUV(const glm::vec2& uv, int rotationAmount, bool flipHorizontal) const;
};

#endif // FACE_BAKER_HPP