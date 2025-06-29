#include "chunk_mesh_generator.hpp"
#include <glm/glm.hpp>
#include <array>
#include <vector>
#include <iostream> // デバッグ用に一時的に追加
#include <limits>   // std::numeric_limits のために追加

// 立方体の基本頂点データ
// Vertex構造体がaoを含むようになったため、初期化を修正
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

// neighborOffsets は chunk_mesh_generator.hpp に移動済み

const std::array<glm::vec3, 6> faceNormals = {
    glm::vec3(0.0f, 0.0f, -1.0f), // 0: Back face (Z-)
    glm::vec3(0.0f, 0.0f, 1.0f),  // 1: Front face (Z+)
    glm::vec3(-1.0f, 0.0f, 0.0f), // 2: Left face (X-)
    glm::vec3(1.0f, 0.0f, 0.0f),  // 3: Right face (X+)
    glm::vec3(0.0f, -1.0f, 0.0f), // 4: Bottom face (Y-)
    glm::vec3(0.0f, 1.0f, 0.0f)   // 5: Top face (Y+)
};

const std::array<glm::vec2, 4> faceUVs = {
    glm::vec2(0.0f, 0.0f), // 左下
    glm::vec2(1.0f, 0.0f), // 右下
    glm::vec2(1.0f, 1.0f), // 右上
    glm::vec2(0.0f, 1.0f)  // 左上
};


inline size_t getVoxelIndex(int x, int y, int z, int chunkSize) {
    if (x < 0 || x >= chunkSize || y < 0 || y >= chunkSize || z < 0 || z >= chunkSize) {
        // 範囲外のアクセスは安全な値を返す
        return std::numeric_limits<size_t>::max(); // 無効なインデックス
    }
    return static_cast<size_t>(x + y * chunkSize + z * chunkSize * chunkSize);
}

// 特定のボクセルの隣接ボクセルを取得するヘルパー関数
// 隣接チャンクも考慮する
bool isVoxelSolid(int x, int y, int z, int chunkSize,
                  const Chunk& currentChunk,
                  const Chunk* neighbor_neg_x, const Chunk* neighbor_pos_x,
                  const Chunk* neighbor_neg_y, const Chunk* neighbor_pos_y,
                  const Chunk* neighbor_neg_z, const Chunk* neighbor_pos_z) {

    if (x >= 0 && x < chunkSize &&
        y >= 0 && y < chunkSize &&
        z >= 0 && z < chunkSize)
    {
        // 現在のチャンク内のボクセル
        return currentChunk.getVoxels()[getVoxelIndex(x, y, z, chunkSize)];
    }
    else // 隣接チャンクのボクセル
    {
        const Chunk* targetChunk = nullptr;
        int targetX = x;
        int targetY = y;
        int targetZ = z;

        if (x < 0) {
            targetChunk = neighbor_neg_x;
            targetX = chunkSize + x; // xは負なので、chunkSize - |x| と同じ
        } else if (x >= chunkSize) {
            targetChunk = neighbor_pos_x;
            targetX = x - chunkSize;
        } else if (y < 0) {
            targetChunk = neighbor_neg_y;
            targetY = chunkSize + y;
        } else if (y >= chunkSize) {
            targetChunk = neighbor_pos_y;
            targetY = 0; // y-1 -> チャンクの最上段のセル
        } else if (z < 0) {
            targetChunk = neighbor_neg_z;
            targetZ = chunkSize + z;
        } else if (z >= chunkSize) {
            targetChunk = neighbor_pos_z;
            targetZ = z - chunkSize;
        }
        
        if (targetChunk != nullptr && 
            targetX >= 0 && targetX < chunkSize &&
            targetY >= 0 && targetY < chunkSize &&
            targetZ >= 0 && targetZ < chunkSize) {
            return targetChunk->getVoxels()[getVoxelIndex(targetX, targetY, targetZ, chunkSize)];
        }
    }
    return false; // 範囲外またはチャンクが存在しない場合はソリッドではないとみなす
}

// 頂点ごとのアンビエントオクルージョンを計算するヘルパー関数
// 参考: https://0fps.net/2013/07/07/meshing-in-a-minecraft-game/
float getAmbientOcclusion(int x, int y, int z, int chunkSize,
                          const Chunk& currentChunk,
                          const Chunk* neighbor_neg_x, const Chunk* neighbor_pos_x,
                          const Chunk* neighbor_neg_y, const Chunk* neighbor_pos_y,
                          const Chunk* neighbor_neg_z, const Chunk* neighbor_pos_z,
                          float cornerDX, float cornerDY, float cornerDZ, // 頂点のボクセル内相対座標 (0.0f or 1.0f)
                          int faceIndex // 面のインデックス (0-5)
                          ) {
    
    // (x, y, z) は現在処理中のボクセルのローカル座標
    // (cornerDX, cornerDY, cornerDZ) は、そのボクセルから見た現在の頂点の相対座標 (0.0f or 1.0f)
    // 例えば、ボクセル(5,5,5) の右手前上の頂点 (5+1.0, 5+1.0, 5+1.0) のAOを計算したい場合
    // x=5, y=5, z=5, cornerDX=1.0f, cornerDY=1.0f, cornerDZ=1.0f となる。

    // 0fps のAO計算ロジックを忠実に再現します。
    // 各面の各頂点において、その頂点に隣接する3つの「影響を与えるボクセル」をチェックする方式。
    
    int side1_dx = 0, side1_dy = 0, side1_dz = 0;
    int side2_dx = 0, side2_dy = 0, side2_dz = 0;
    int corner_dx = 0, corner_dy = 0, corner_dz = 0;

    // 面の法線方向 (faceIndex) と頂点の相対位置 (cornerDX, cornerDY, cornerDZ) に応じて、
    // チェックする3つのボクセルを決定します。
    
    // X- 面 (左) のAO (Normal: (-1, 0, 0))
    if (faceIndex == 2) { 
        side1_dy = (cornerDY == 0.0f) ? -1 : 1; // Y方向の隣接ボクセル
        side2_dz = (cornerDZ == 0.0f) ? -1 : 1; // Z方向の隣接ボクセル
        corner_dy = side1_dy;
        corner_dz = side2_dz;
    }
    // X+ 面 (右) のAO (Normal: (1, 0, 0))
    else if (faceIndex == 3) {
        // 現在のボクセル座標xに対して、隣接ボクセルは常にx+1の方向にある
        x = x + 1; 
        side1_dy = (cornerDY == 0.0f) ? -1 : 1;
        side2_dz = (cornerDZ == 0.0f) ? -1 : 1;
        corner_dy = side1_dy;
        corner_dz = side2_dz;
    }
    // Y- 面 (底) のAO (Normal: (0, -1, 0))
    else if (faceIndex == 4) {
        side1_dx = (cornerDX == 0.0f) ? -1 : 1;
        side2_dz = (cornerDZ == 0.0f) ? -1 : 1;
        corner_dx = side1_dx;
        corner_dz = side2_dz;
    }
    // Y+ 面 (上) のAO (Normal: (0, 1, 0))
    else if (faceIndex == 5) {
        // 現在のボクセル座標yに対して、隣接ボクセルは常にy+1の方向にある
        y = y + 1;
        side1_dx = (cornerDX == 0.0f) ? -1 : 1;
        side2_dz = (cornerDZ == 0.0f) ? -1 : 1;
        corner_dx = side1_dx;
        corner_dz = side2_dz;
    }
    // Z- 面 (奥) のAO (Normal: (0, 0, -1))
    else if (faceIndex == 0) {
        side1_dx = (cornerDX == 0.0f) ? -1 : 1;
        side2_dy = (cornerDY == 0.0f) ? -1 : 1;
        corner_dx = side1_dx;
        corner_dy = side2_dy;
    }
    // Z+ 面 (手前) のAO (Normal: (0, 0, 1))
    else if (faceIndex == 1) {
        // 現在のボクセル座標zに対して、隣接ボクセルは常にz+1の方向にある
        z = z + 1;
        side1_dx = (cornerDX == 0.0f) ? -1 : 1;
        side2_dy = (cornerDY == 0.0f) ? -1 : 1;
        corner_dx = side1_dx;
        corner_dy = side2_dy;
    }

    // 各ボクセルのソリッド状態をチェック
    bool side1_solid = isVoxelSolid(x + side1_dx, y + side1_dy, z + side1_dz, chunkSize, currentChunk, neighbor_neg_x, neighbor_pos_x, neighbor_neg_y, neighbor_pos_y, neighbor_neg_z, neighbor_pos_z);
    bool side2_solid = isVoxelSolid(x + side2_dx, y + side2_dy, z + side2_dz, chunkSize, currentChunk, neighbor_neg_x, neighbor_pos_x, neighbor_neg_y, neighbor_pos_y, neighbor_neg_z, neighbor_pos_z);
    bool corner_solid = isVoxelSolid(x + corner_dx, y + corner_dy, z + corner_dz, chunkSize, currentChunk, neighbor_neg_x, neighbor_pos_x, neighbor_neg_y, neighbor_pos_y, neighbor_neg_z, neighbor_pos_z);

    // 0fps のAO値を計算
    // 3つのボクセルが全て埋まっている場合
    if (side1_solid && side2_solid && corner_solid) {
        return 0.0f; // 最も暗い
    }
    // 2つのボクセルが埋まっている場合
    else if ((side1_solid && side2_solid) || (side1_solid && corner_solid) || (side2_solid && corner_solid)) {
        return 1.0f;
    }
    // 1つのボクセルが埋まっている場合
    else if (side1_solid || side2_solid || corner_solid) {
        return 2.0f;
    }
    // 全て空の場合
    else {
        return 3.0f; // 最も明るい
    }
}


ChunkMeshData ChunkMeshGenerator::generateMesh(const Chunk &chunk,
                                              const Chunk* neighbor_neg_x,
                                              const Chunk* neighbor_pos_x,
                                              const Chunk* neighbor_neg_y,
                                              const Chunk* neighbor_pos_y,
                                              const Chunk* neighbor_neg_z,
                                              const Chunk* neighbor_pos_z)
{
    ChunkMeshData meshData;
    int chunkSize = chunk.getSize();
    const std::vector<bool>& voxels = chunk.getVoxels();

    meshData.vertices.reserve(chunkSize * chunkSize * chunkSize * 4 * 6);
    meshData.indices.reserve(chunkSize * chunkSize * chunkSize * 6 * 6);

    for (int z = 0; z < chunkSize; ++z)
    {
        for (int y = 0; y < chunkSize; ++y)
        {
            for (int x = 0; x < chunkSize; ++x)
            {
                if (voxels[getVoxelIndex(x, y, z, chunkSize)])
                {
                    size_t currentVertexCount = meshData.vertices.size();

                    for (int i = 0; i < 6; ++i) // 6面すべてについてループ
                    {
                        glm::ivec3 offset = neighborOffsets[i];
                        int neighborX = x + offset.x;
                        int neighborY = y + offset.y;
                        int neighborZ = z + offset.z;

                        bool renderFace = true;

                        // 隣接ボクセルが現在のチャンク内にあるかチェック
                        if (neighborX >= 0 && neighborX < chunkSize &&
                            neighborY >= 0 && neighborY < chunkSize &&
                            neighborZ >= 0 && neighborZ < chunkSize)
                        {
                            // 現在のチャンク内に隣接ボクセルがある場合
                            if (voxels[getVoxelIndex(neighborX, neighborY, neighborZ, chunkSize)])
                            {
                                renderFace = false; // 隣接ボクセルがあるので、この面は不要
                            }
                        }
                        else // 隣接ボクセルが現在のチャンクの境界外にある場合 (つまり、隣接チャンク内か世界の端)
                        {
                            const Chunk* adjacentChunk = nullptr;
                            int relativeNeighborX = neighborX; // 隣接チャンク内での相対座標
                            int relativeNeighborY = neighborY;
                            int relativeNeighborZ = neighborZ;

                            if (offset.x == -1) {
                                adjacentChunk = neighbor_neg_x;
                                relativeNeighborX = chunkSize - 1;
                            } else if (offset.x == 1) {
                                adjacentChunk = neighbor_pos_x;
                                relativeNeighborX = 0;
                            } else if (offset.y == -1) {
                                adjacentChunk = neighbor_neg_y;
                                relativeNeighborY = chunkSize - 1;
                            } else if (offset.y == 1) {
                                adjacentChunk = neighbor_pos_y;
                                relativeNeighborY = 0;
                            } else if (offset.z == -1) {
                                adjacentChunk = neighbor_neg_z;
                                relativeNeighborZ = chunkSize - 1;
                            } else if (offset.z == 1) {
                                adjacentChunk = neighbor_pos_z;
                                relativeNeighborZ = 0;
                            }
                            
                            if (adjacentChunk != nullptr && 
                                relativeNeighborX >= 0 && relativeNeighborX < chunkSize &&
                                relativeNeighborY >= 0 && relativeNeighborY < chunkSize &&
                                relativeNeighborZ >= 0 && relativeNeighborZ < chunkSize) {
                                if (adjacentChunk->getVoxels()[getVoxelIndex(relativeNeighborX, relativeNeighborY, relativeNeighborZ, chunkSize)]) {
                                    renderFace = false;
                                }
                            }
                        }

                        if (renderFace)
                        {
                            glm::vec3 currentFaceNormal = faceNormals[i];

                            for (int v_idx = 0; v_idx < 4; ++v_idx) // 4つの頂点についてループ
                            {
                                unsigned int baseIdx = cubeFaceBaseIndices[i][v_idx];
                                Vertex baseVertex = baseCubeVertices[baseIdx];

                                Vertex newVertex = baseVertex;
                                newVertex.x += x;
                                newVertex.y += y;
                                newVertex.z += z;

                                newVertex.u = faceUVs[v_idx].x;
                                newVertex.v = faceUVs[v_idx].y;

                                newVertex.nx = currentFaceNormal.x;
                                newVertex.ny = currentFaceNormal.y;
                                newVertex.nz = currentFaceNormal.z;

                                // AO値を計算して設定
                                // baseVertex.x, baseVertex.y, baseVertex.z はボクセル内の相対頂点位置 (0.0 or 1.0)
                                newVertex.ao = getAmbientOcclusion(x, y, z, chunkSize, 
                                                                 chunk, 
                                                                 neighbor_neg_x, neighbor_pos_x,
                                                                 neighbor_neg_y, neighbor_pos_y,
                                                                 neighbor_neg_z, neighbor_pos_z,
                                                                 baseVertex.x, baseVertex.y, baseVertex.z, // 頂点の相対位置を渡す
                                                                 i // 面のインデックスも渡す
                                                                 );

                                meshData.vertices.push_back(newVertex);
                            }

                            meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + 0);
                            meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + 1);
                            meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + 2);

                            meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + 0);
                            meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + 2);
                            meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + 3);

                            currentVertexCount += 4;
                        }
                    }
                }
            }
        }
    }
    return meshData;
}