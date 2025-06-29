#include "chunk_mesh_generator.hpp"
#include <glm/glm.hpp>
#include <array>
#include <vector>
#include <iostream> // デバッグ用に一時的に追加
#include <limits>   // std::numeric_limits のために追加
#include <random>   // 乱数生成のために追加

// 立方体の基本頂点データ (変更なし)
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
    glm::vec2(0.0f, 0.0f), // 左下 (bottom-left)
    glm::vec2(1.0f, 0.0f), // 右下 (bottom-right)
    glm::vec2(1.0f, 1.0f), // 右上 (top-right)
    glm::vec2(0.0f, 1.0f)  // 左上 (top-left)
};

inline size_t getVoxelIndex(int x, int y, int z, int chunkSize)
{
    if (x < 0 || x >= chunkSize || y < 0 || y >= chunkSize || z < 0 || z >= chunkSize)
    {
        // 範囲外のアクセスは安全な値を返す
        return std::numeric_limits<size_t>::max(); // 無効なインデックス
    }
    return static_cast<size_t>(x + y * chunkSize + z * chunkSize * chunkSize);
}

// 特定のボクセルの隣接ボクセルを取得するヘルパー関数
// 隣接チャンクも考慮する
bool isVoxelSolid(int x, int y, int z, int chunkSize,
                  const Chunk &currentChunk,
                  const Chunk *neighbor_neg_x, const Chunk *neighbor_pos_x,
                  const Chunk *neighbor_neg_y, const Chunk *neighbor_pos_y,
                  const Chunk *neighbor_neg_z, const Chunk *neighbor_pos_z)
{

    if (x >= 0 && x < chunkSize &&
        y >= 0 && y < chunkSize &&
        z >= 0 && z < chunkSize)
    {
        // 現在のチャンク内のボクセル
        return currentChunk.getVoxels()[getVoxelIndex(x, y, z, chunkSize)];
    }
    else // 隣接チャンクのボクセル
    {
        const Chunk *targetChunk = nullptr;
        int targetX = x;
        int targetY = y;
        int targetZ = z;

        if (x < 0)
        {
            targetChunk = neighbor_neg_x;
            targetX = chunkSize + x;
        }
        else if (x >= chunkSize)
        {
            targetChunk = neighbor_pos_x;
            targetX = x - chunkSize;
        }
        else if (y < 0)
        {
            targetChunk = neighbor_neg_y;
            targetY = chunkSize + y;
        }
        else if (y >= chunkSize)
        {
            targetChunk = neighbor_pos_y;
            targetY = y - chunkSize; 
        }
        else if (z < 0)
        {
            targetChunk = neighbor_neg_z;
            targetZ = chunkSize + z;
        }
        else if (z >= chunkSize)
        {
            targetChunk = neighbor_pos_z;
            targetZ = z - chunkSize;
        }

        if (targetChunk != nullptr &&
            targetX >= 0 && targetX < chunkSize &&
            targetY >= 0 && targetY < chunkSize &&
            targetZ >= 0 && targetZ < chunkSize)
        {
            return targetChunk->getVoxels()[getVoxelIndex(targetX, targetY, targetZ, chunkSize)];
        }
    }
    return false; // 範囲外またはチャンクが存在しない場合はソリッドではないとみなす
}

// 頂点ごとのアンビエントオクルージョンを計算するヘルパー関数
float getAmbientOcclusion(int x, int y, int z, int chunkSize,
                          const Chunk &currentChunk,
                          const Chunk *neighbor_neg_x, const Chunk *neighbor_pos_x,
                          const Chunk *neighbor_neg_y, const Chunk *neighbor_pos_y,
                          const Chunk *neighbor_neg_z, const Chunk *neighbor_pos_z,
                          float cornerDX, float cornerDY, float cornerDZ,
                          int faceIndex)
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
    bool side1_solid = isVoxelSolid(x + side1_dx, y + side1_dy, z + side1_dz, chunkSize, currentChunk, neighbor_neg_x, neighbor_pos_x, neighbor_neg_y, neighbor_pos_y, neighbor_neg_z, neighbor_pos_z);
    bool side2_solid = isVoxelSolid(x + side2_dx, y + side2_dy, z + side2_dz, chunkSize, currentChunk, neighbor_neg_x, neighbor_pos_x, neighbor_neg_y, neighbor_pos_y, neighbor_neg_z, neighbor_pos_z);
    bool corner_solid = isVoxelSolid(x + corner_dx, y + corner_dy, z + corner_dz, chunkSize, currentChunk, neighbor_neg_x, neighbor_pos_x, neighbor_neg_y, neighbor_pos_y, neighbor_neg_z, neighbor_pos_z);

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

ChunkMeshData ChunkMeshGenerator::generateMesh(const Chunk &chunk,
                                               const Chunk *neighbor_neg_x,
                                               const Chunk *neighbor_pos_x,
                                               const Chunk *neighbor_neg_y,
                                               const Chunk *neighbor_pos_y,
                                               const Chunk *neighbor_neg_z,
                                               const Chunk *neighbor_pos_z)
{
    ChunkMeshData meshData;
    int chunkSize = chunk.getSize();
    const std::vector<bool> &voxels = chunk.getVoxels();

    // チャンクのワールド座標を取得（ChunkクラスにgetCoord()メソッドがあると仮定）
    // もしChunkクラスにgetCoord()がない場合は、引数として渡すか、別の方法でチャンクのユニークな識別子を取得してください。
    glm::ivec3 chunkCoord = chunk.getCoord(); 
    
    // チャンクの座標をシードとして使用することで、同じチャンクは常に同じ乱数パターンを生成します。
    // 座標が負になる可能性があるので、大きな素数で乗算して衝突を減らします。
    std::seed_seq seed_seq{
        static_cast<std::uint32_t>(chunkCoord.x),
        static_cast<std::uint32_t>(chunkCoord.y),
        static_cast<std::uint32_t>(chunkCoord.z)
    };
    std::mt19937 rng(seed_seq);
    
    // 回転用: 0=0deg, 1=90deg, 2=180deg, 3=270deg
    std::uniform_int_distribution<int> rotation_dist(0, 3); 
    // 反転用: 0=反転なし, 1=水平反転
    std::uniform_int_distribution<int> flip_dist(0, 1); 

    meshData.vertices.reserve(chunkSize * chunkSize * chunkSize * 4 * 6);
    meshData.indices.reserve(chunkSize * chunkSize * chunkSize * 6 * 6);

    for (int z = 0; z < chunkSize; ++z)
    {
        for (int y = 0; y < chunkSize; ++y)
        {
            for (int x = 0; x < chunkSize; ++x)
            {
                // ここでボクセルの種類を考慮し、草ブロックの場合のみUVを回転・反転させるといった制御も可能
                // if (voxels[getVoxelIndex(x, y, z, chunkSize)] && chunk.getVoxelType(x,y,z) == BlockType::GRASS) { ... }
                if (voxels[getVoxelIndex(x, y, z, chunkSize)])
                {
                    size_t currentVertexCount = meshData.vertices.size();

                    // このボクセルに適用するUV回転パターンと反転パターンをランダムに決定
                    // ボクセルごとに同じパターンが適用されるように、面ループの外で決定
                    // AOの計算もこのボクセルに対して行われるため、ボクセルごとの乱数で問題ありません。
                    int rotationAmount = rotation_dist(rng);
                    bool flipHorizontal = (flip_dist(rng) == 1);

                    for (int i = 0; i < 6; ++i) // 6面すべてについてループ
                    {
                        glm::ivec3 offset = neighborOffsets[i];
                        int neighborX = x + offset.x;
                        int neighborY = y + offset.y;
                        int neighborZ = z + offset.z;

                        bool renderFace = true;
                        if (isVoxelSolid(neighborX, neighborY, neighborZ, chunkSize,
                                         chunk,
                                         neighbor_neg_x, neighbor_pos_x,
                                         neighbor_neg_y, neighbor_pos_y,
                                         neighbor_neg_z, neighbor_pos_z))
                        {
                            renderFace = false; // 隣接ボクセルがソリッドなので、この面は描画不要
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

                                // ここで元のUV座標を取得
                                glm::vec2 uv = faceUVs[v_idx];

                                // まず回転を適用
                                switch (rotationAmount) {
                                    case 1: // 90度回転 (反時計回り)
                                        uv = glm::vec2(1.0f - uv.y, uv.x);
                                        break;
                                    case 2: // 180度回転
                                        uv = glm::vec2(1.0f - uv.x, 1.0f - uv.y);
                                        break;
                                    case 3: // 270度回転 (反時計回り)
                                        uv = glm::vec2(uv.y, 1.0f - uv.x);
                                        break;
                                    default: // 0度回転 (case 0)
                                        // 何もしない
                                        break;
                                }

                                // 次に水平反転を適用 (U座標のみ反転)
                                if (flipHorizontal) {
                                    uv.x = 1.0f - uv.x;
                                }
                                // 必要であれば垂直反転 (uv.y = 1.0f - uv.y;) も追加可能

                                newVertex.u = uv.x;
                                newVertex.v = uv.y;

                                newVertex.nx = currentFaceNormal.x;
                                newVertex.ny = currentFaceNormal.y;
                                newVertex.nz = currentFaceNormal.z;

                                // AO値を計算して設定
                                newVertex.ao = getAmbientOcclusion(x, y, z, chunkSize,
                                                                   chunk,
                                                                   neighbor_neg_x, neighbor_pos_x,
                                                                   neighbor_neg_y, neighbor_pos_y,
                                                                   neighbor_neg_z, neighbor_pos_z,
                                                                   baseVertex.x, baseVertex.y, baseVertex.z,
                                                                   i);

                                meshData.vertices.push_back(newVertex);
                            }

                            // インデックスは常に同じ順序
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