#include "chunk_mesh_generator.hpp"
#include <glm/glm.hpp>
#include <array>
#include <vector>

// 立方体の基本頂点データ
// 各面の「基本頂点」は、面の形を定義する最小限の4つの頂点と考える
const std::vector<Vertex> baseCubeVertices = {
    {-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f}, // 0: Back-bottom-left (Z-)
    {0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f},  // 1: Back-bottom-right (Z-)
    {0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f},   // 2: Back-top-right (Z-)
    {-0.5f, 0.5f, -0.5f, 0.0f, 0.0f, 1.0f},  // 3: Back-top-left (Z-)
    {0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f},    // 4: Front-top-right (Z+)
    {0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 1.0f},   // 5: Front-bottom-right (Z+)
    {-0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 1.0f},  // 6: Front-bottom-left (Z+)
    {-0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f}    // 7: Front-top-left (Z+)
};

// 各面ごとの基本頂点インデックス（baseCubeVerticesへの参照）
// 各面は4つのユニークな頂点インデックスから構成されます。
// ここでの順序は、面を**外側から見たときに**、
// CCW（反時計回り）になるように定義されています。
// 具体的には、各面の最初の頂点（[0]）を基準とし、
// それを起点に反時計回りに他の頂点を並べます。
const std::array<std::array<unsigned int, 4>, 6> cubeFaceBaseIndices = {
    // 0: Back face (Z-): 法線(0,0,-1)
    // 外側から見てCCW: 0, 3, 2, 1 (左下、左上、右上、右下)
    std::array<unsigned int, 4>{0, 3, 2, 1},

    // 1: Front face (Z+): 法線(0,0,1)
    // 外側から見てCCW: 6, 5, 4, 7 (左下、右下、右上、左上) -- ここを修正しました
    std::array<unsigned int, 4>{6, 5, 4, 7},

    // 2: Left face (X-): 法線(-1,0,0)
    // 外側から見てCCW: 0, 6, 7, 3 (左下、左上、右上、右下)
    std::array<unsigned int, 4>{0, 6, 7, 3},

    // 3: Right face (X+): 法線(1,0,0)
    // 外側から見てCCW: 1, 2, 4, 5 (左下、左上、右上、右下)
    std::array<unsigned int, 4>{1, 2, 4, 5},

    // 4: Bottom face (Y-): 法線(0,-1,0)
    // 外側から見てCCW: 0, 1, 5, 6 (左下、左上、右上、右下)
    std::array<unsigned int, 4>{0, 1, 5, 6},

    // 5: Top face (Y+): 法線(0,1,0)
    // 外側から見てCCW: 3, 7, 4, 2 (左下、左上、右上、右下)
    std::array<unsigned int, 4>{3, 7, 4, 2}
};

// 各面に対応する隣接ボクセルのオフセット (x, y, z)
const std::array<glm::ivec3, 6> neighborOffsets = {
    glm::ivec3(0, 0, -1), // Back face (Z-)
    glm::ivec3(0, 0, 1),  // Front face (Z+)
    glm::ivec3(-1, 0, 0), // Left face (X-)
    glm::ivec3(1, 0, 0),  // Right face (X+)
    glm::ivec3(0, -1, 0), // Bottom face (Y-)
    glm::ivec3(0, 1, 0)   // Top face (Y+)
};

ChunkMeshData ChunkMeshGenerator::generateMesh(const Chunk &chunk, float cubeSpacing)
{
    ChunkMeshData meshData;
    int chunkSize = chunk.getSize();

    // 最適化: 事前にベクタの容量を予約
    meshData.vertices.reserve(chunkSize * chunkSize * chunkSize * 4 * 6);
    meshData.indices.reserve(chunkSize * chunkSize * chunkSize * 6 * 6);

    for (int x = 0; x < chunkSize; ++x)
    {
        for (int y = 0; y < chunkSize; ++y)
        {
            for (int z = 0; z < chunkSize; ++z)
            {
                if (chunk.getVoxel(x, y, z))
                { // 現在のボクセルが存在する場合
                    size_t currentVertexCount = meshData.vertices.size();

                    // 6つの各面についてチェック
                    for (int i = 0; i < 6; ++i)
                    {
                        glm::ivec3 offset = neighborOffsets[i];
                        int neighborX = x + offset.x;
                        int neighborY = y + offset.y;
                        int neighborZ = z + offset.z;

                        bool renderFace = true;
                        // 隣接するボクセルがチャンク内にあるか、かつ不透明な場合、面は描画しない
                        if (neighborX >= 0 && neighborX < chunkSize &&
                            neighborY >= 0 && neighborY < chunkSize &&
                            neighborZ >= 0 && neighborZ < chunkSize)
                        {
                            if (chunk.getVoxel(neighborX, neighborY, neighborZ))
                            {
                                renderFace = false;
                            }
                        }

                        if (renderFace)
                        {
                            // この面を描画する必要がある場合
                            // 各面の4つの基本頂点を追加
                            // cubeFaceBaseIndices[i] は、外側から見てCCWになるように並べられた4つの頂点のインデックス
                            // V0 (0) V1 (1) V2 (2) V3 (3)
                            for (unsigned int baseIdx : cubeFaceBaseIndices[i])
                            {
                                Vertex baseVertex = baseCubeVertices[baseIdx];
                                Vertex newVertex = baseVertex;
                                // ボクセル位置にオフセットを適用し、間隔を考慮
                                newVertex.x += x * cubeSpacing;
                                newVertex.y += y * cubeSpacing;
                                newVertex.z += z * cubeSpacing;
                                meshData.vertices.push_back(newVertex);
                            }

                            // 2つの三角形を形成する6つのインデックスを追加 (CCW順)
                            // 追加された4つの頂点に対して、相対インデックスは 0, 1, 2, 3 となる
                            // 三角形1: V0-V1-V2
                            meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + 0); // V0
                            meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + 1); // V1
                            meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + 2); // V2

                            // 三角形2: V0-V2-V3
                            meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + 0); // V0
                            meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + 2); // V2
                            meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + 3); // V3

                            // currentVertexCount は次の面のために更新
                            currentVertexCount += 4;
                        }
                    }
                }
            }
        }
    }

    return meshData;
}