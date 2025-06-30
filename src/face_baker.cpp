#include "face_baker.hpp"
#include "mesh_types.hpp" // FaceBaker.hpp でインクルードされるため、ここでの明示的なインクルードは必須ではありませんが、明示的に含めても問題ありません。

// 定義は引き続き.cppファイルに保持
const std::vector<Vertex> baseCubeVertices = {
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, // 0: Back-bottom-left (Z-)
    {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, // 1: Back-bottom-right (Z-)
    {1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, // 2: Back-top-right (Z-)
    {0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, // 3: Back-top-left (Z-)
    {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, // 4: Front-top-right (Z+)
    {1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, // 5: Front-bottom-right (Z+)
    {0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, // 6: Front-bottom-left (Z+)
    {0.0f, 1.0f, 1.0f, 0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}  // 7: Front-top-left (Z+)
};

const std::array<std::array<unsigned int, 4>, 6> cubeFaceBaseIndices = {
    std::array<unsigned int, 4>{0, 3, 2, 1}, // 0: Back face (Z-)
    std::array<unsigned int, 4>{6, 5, 4, 7}, // 1: Front face (Z+)
    std::array<unsigned int, 4>{0, 6, 7, 3}, // 2: Left face (X-)
    std::array<unsigned int, 4>{1, 2, 4, 5}, // 3: Right face (X+)
    std::array<unsigned int, 4>{0, 1, 5, 6}, // 4: Bottom face (Y-)
    std::array<unsigned int, 4>{3, 7, 4, 2}  // 5: Top face (Y+)
};

const std::array<glm::vec3, 6> faceNormals = {
    glm::vec3(0.0f, 0.0f, -1.0f), // 0: Back face (Z-)
    glm::vec3(0.0f, 0.0f, 1.0f),  // 1: Front face (Z+)
    glm::vec3(-1.0f, 0.0f, 0.0f), // 2: Left face (X-)
    glm::vec3(1.0f, 0.0f, 0.0f),  // 3: Right face (X+)
    glm::vec3(0.0f, -1.0f, 0.0f), // 4: Bottom face (Y-)
    glm::vec3(0.0f, 1.0f, 0.0f)   // 5: Top face (Y+)
};

const std::array<glm::vec2, 4> faceUVs = {
    glm::vec2(0.0f, 0.0f), // 左下 (bottom-left)
    glm::vec2(1.0f, 0.0f), // 右下 (bottom-right)
    glm::vec2(1.0f, 1.0f), // 右上 (top-right)
    glm::vec2(0.0f, 1.0f)  // 左上 (top-left)
};


FaceBaker::FaceBaker(const VoxelAccessor& accessor, int chunkSize)
    : voxelAccessor_(accessor), chunkSize_(chunkSize)
{
}

float FaceBaker::calculateAmbientOcclusion(int x, int y, int z,
                                         float cornerDX, float cornerDY, float cornerDZ,
                                         int faceIndex) const
{
    int side1_dx = 0, side1_dy = 0, side1_dz = 0;
    int side2_dx = 0, side2_dy = 0, side2_dz = 0;
    int corner_dx = 0, corner_dy = 0, corner_dz = 0;

    // 面の法線方向 (faceIndex) と頂点の相対位置 (cornerDX, cornerDY, cornerDZ) に応じて、
    // チェックする3つのボクセルを決定します。

    // X- 面 (左) のAO (Normal: (-1, 0, 0))
    if (faceIndex == 2)
    {
        side1_dy = (cornerDY == 0.0f) ? -1 : 1;
        side2_dz = (cornerDZ == 0.0f) ? -1 : 1;
        corner_dy = side1_dy;
        corner_dz = side2_dz;
    }
    // X+ 面 (右) のAO (Normal: (1, 0, 0))
    else if (faceIndex == 3)
    {
        x = x + 1; // 隣接ボクセルは常にx+1の方向にある
        side1_dy = (cornerDY == 0.0f) ? -1 : 1;
        side2_dz = (cornerDZ == 0.0f) ? -1 : 1;
        corner_dy = side1_dy;
        corner_dz = side2_dz;
    }
    // Y- 面 (底) のAO (Normal: (0, -1, 0))
    else if (faceIndex == 4)
    {
        side1_dx = (cornerDX == 0.0f) ? -1 : 1;
        side2_dz = (cornerDZ == 0.0f) ? -1 : 1;
        corner_dx = side1_dx;
        corner_dz = side2_dz;
    }
    // Y+ 面 (上) のAO (Normal: (0, 1, 0))
    else if (faceIndex == 5)
    {
        y = y + 1; // 隣接ボクセルは常にy+1の方向にある
        side1_dx = (cornerDX == 0.0f) ? -1 : 1;
        side2_dz = (cornerDZ == 0.0f) ? -1 : 1;
        corner_dx = side1_dx;
        corner_dz = side2_dz;
    }
    // Z- 面 (奥) のAO (Normal: (0, 0, -1))
    else if (faceIndex == 0)
    {
        side1_dx = (cornerDX == 0.0f) ? -1 : 1;
        side2_dy = (cornerDY == 0.0f) ? -1 : 1;
        corner_dx = side1_dx;
        corner_dy = side2_dy;
    }
    // Z+ 面 (手前) のAO (Normal: (0, 0, 1))
    else if (faceIndex == 1)
    {
        z = z + 1; // 隣接ボクセルは常にz+1の方向にある
        side1_dx = (cornerDX == 0.0f) ? -1 : 1;
        side2_dy = (cornerDY == 0.0f) ? -1 : 1;
        corner_dx = side1_dx;
        corner_dy = side2_dy;
    }

    // 各ボクセルのソリッド状態をチェック
    bool side1_solid = voxelAccessor_.isSolid(x + side1_dx, y + side1_dy, z + side1_dz);
    bool side2_solid = voxelAccessor_.isSolid(x + side2_dx, y + side2_dy, z + side2_dz);
    bool corner_solid = voxelAccessor_.isSolid(x + corner_dx, y + corner_dy, z + corner_dz);

    // 0fps のAO値を計算
    if (side1_solid && side2_solid && corner_solid)
    {
        return 0.0f; // 最も暗い
    }
    else if ((side1_solid && side2_solid) || (side1_solid && corner_solid) || (side2_solid && corner_solid))
    {
        return 1.0f;
    }
    else if (side1_solid || side2_solid || corner_solid)
    {
        return 2.0f;
    }
    else
    {
        return 3.0f; // 最も明るい
    }
}

glm::vec2 FaceBaker::transformUV(const glm::vec2& uv, int rotationAmount, bool flipHorizontal) const
{
    glm::vec2 transformedUV = uv;

    // まず回転を適用
    switch (rotationAmount)
    {
    case 1: // 90度回転 (反時計回り)
        transformedUV = glm::vec2(1.0f - uv.y, uv.x);
        break;
    case 2: // 180度回転
        transformedUV = glm::vec2(1.0f - uv.x, 1.0f - uv.y);
        break;
    case 3: // 270度回転 (反時計回り)
        transformedUV = glm::vec2(uv.y, 1.0f - uv.x);
        break;
    default: // 0度回転 (case 0)
        // 何もしない
        break;
    }

    // 次に水平反転を適用 (U座標のみ反転)
    if (flipHorizontal)
    {
        transformedUV.x = 1.0f - transformedUV.x;
    }
    return transformedUV;
}

void FaceBaker::bakeFace(ChunkMeshData& meshData, int x, int y, int z, int faceIndex,
                         int rotationAmount, bool flipHorizontal)
{
    size_t currentVertexCount = meshData.vertices.size();
    glm::vec3 currentFaceNormal = faceNormals[faceIndex];

    for (int v_idx = 0; v_idx < 4; ++v_idx) // 4つの頂点についてループ
    {
        unsigned int baseIdx = cubeFaceBaseIndices[faceIndex][v_idx];
        Vertex baseVertex = baseCubeVertices[baseIdx];

        Vertex newVertex = baseVertex;
        newVertex.x += x;
        newVertex.y += y;
        newVertex.z += z;

        // ここで元のUV座標を取得し、変換関数を適用
        glm::vec2 uv = transformUV(faceUVs[v_idx], rotationAmount, flipHorizontal);

        newVertex.u = uv.x;
        newVertex.v = uv.y;

        newVertex.nx = currentFaceNormal.x;
        newVertex.ny = currentFaceNormal.y;
        newVertex.nz = currentFaceNormal.z;

        // AO値を計算して設定
        newVertex.ao = calculateAmbientOcclusion(x, y, z,
                                                 baseVertex.x, baseVertex.y, baseVertex.z,
                                                 faceIndex);

        meshData.vertices.push_back(newVertex);
    }

    // インデックスは常に同じ順序
    meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + 0);
    meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + 1);
    meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + 2);

    meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + 0);
    meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + 2);
    meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + 3);
}