// src/chunk_mesh_generator.cpp
#include "chunk_mesh_generator.hpp"
#include <glm/glm.hpp>
#include <array> // std::array を使うため

// 各面の頂点インデックスの定義
// 注意: これは立方体の全体インデックスではなく、特定の面を構成するインデックスです。
// 通常、OpenGLは右手座標系で、CCW（反時計回り）が前面と見なされます。
// 面の定義順序は、見た目の法線が外側を向くように調整されています。

// 各面ごとの頂点オフセットとインデックス
// 1つの面は2つの三角形 (6つのインデックス) で構成されます。
// Vertexデータは0.5を基準とした立方体座標
// {x, y, z, r, g, b}

// 各面のデータ構造を定義
struct Face
{
    std::array<int, 6> indices; // 2つの三角形 (6インデックス)
};

// 各面に対応する頂点オフセット (基本立方体の頂点に対するオフセット)
// 立方体は中心を(0,0,0)とし、各軸の範囲は-0.5から+0.5
// 各面の法線方向に対応するチェックを行うための順序
// 後ろ(Z-), 前(Z+), 下(Y-), 上(Y+), 左(X-), 右(X+)

// 立方体の基本頂点データ
const std::vector<Vertex> baseCubeVertices = {
    {-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f}, // 0: Back-bottom-left (黒)
    {0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f},  // 1: Back-bottom-right (赤)
    {0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f},   // 2: Back-top-right (緑)
    {-0.5f, 0.5f, -0.5f, 0.0f, 0.0f, 1.0f},  // 3: Back-top-left (青)
    {0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f},    // 4: Front-top-right (黄)
    {0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 1.0f},   // 5: Front-bottom-right (シアン)
    {-0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 1.0f},  // 6: Front-bottom-left (マゼンタ)
    {-0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f}    // 7: Front-top-left (グレー)
};

// 各面ごとのインデックス（頂点データへの参照）
// ここでは、各面がどの基本立方体頂点によって構成されるかを定義します。
// 例: Back face は 0,1,2,3 の頂点を使用。
// OpenGLの右手座標系とCCW（反時計回り）ルールに従い、法線が外側を向くようにインデックスを定義。
const std::array<std::array<unsigned int, 6>, 6> cubeFaces = {
    // Back face (Z- / 奥)
    std::array<unsigned int, 6>{0, 3, 2, 0, 2, 1}, // 0: (-0.5,-0.5,-0.5), (-0.5,0.5,-0.5), (0.5,0.5,-0.5), (0.5,-0.5,-0.5)

    // Front face (Z+ / 手前)
    std::array<unsigned int, 6>{6, 5, 4, 4, 7, 6}, // 元のコードのZ+インデックス

    // Left face (X- / 左)
    std::array<unsigned int, 6>{0, 6, 3, 3, 6, 7}, // 2: (-0.5,-0.5,-0.5), (-0.5,-0.5,0.5), (-0.5,0.5,0.5), (-0.5,0.5,-0.5)

    // Right face (X+ / 右)
    std::array<unsigned int, 6>{1, 2, 4, 1, 4, 5}, // 3: (0.5,-0.5,-0.5), (0.5,0.5,-0.5), (0.5,0.5,0.5), (0.5,-0.5,0.5)

    // Bottom face (Y- / 下)
    std::array<unsigned int, 6>{0, 1, 5, 0, 5, 6}, // 4: (-0.5,-0.5,-0.5), (0.5,-0.5,-0.5), (0.5,-0.5,0.5), (-0.5,-0.5,0.5)

    // Top face (Y+ / 上)
    std::array<unsigned int, 6>{3, 7, 4, 3, 4, 2} // 5: (-0.5,0.5,-0.5), (-0.5,0.5,0.5), (0.5,0.5,0.5), (0.5,0.5,-0.5)
};

// 各面に対応する隣接ボクセルのオフセット (x, y, z)
// この順序は cubeFaces と対応している必要があります！
// 例: {0, 0, -1} は Z- 方向 (Back face) をチェック
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

    // チャンク内の各ボクセルを走査
    for (int x = 0; x < chunkSize; ++x)
    {
        for (int y = 0; y < chunkSize; ++y)
        {
            for (int z = 0; z < chunkSize; ++z)
            {
                if (chunk.getVoxel(x, y, z))
                { // 現在のボクセルが存在する場合
                    // 現在のメッシュの頂点数（インデックスのオフセットに使う）
                    size_t currentVertexCount = meshData.vertices.size();

                    // 6つの各面についてチェック
                    for (int i = 0; i < 6; ++i)
                    {
                        glm::ivec3 offset = neighborOffsets[i];
                        int neighborX = x + offset.x;
                        int neighborY = y + offset.y;
                        int neighborZ = z + offset.z;

                        // 隣接する位置にボクセルがあるか、またはチャンクの境界外であるかを確認
                        bool renderFace = true;
                        // チャンク境界内の隣接ボクセルチェック
                        if (neighborX >= 0 && neighborX < chunkSize &&
                            neighborY >= 0 && neighborY < chunkSize &&
                            neighborZ >= 0 && neighborZ < chunkSize)
                        {
                            if (chunk.getVoxel(neighborX, neighborY, neighborZ))
                            {
                                renderFace = false; // 隣接するボクセルがあれば、この面は描画しない
                            }
                        }
                        // チャンクの境界外の場合は、常に面を描画する（空中に面が出ているため）
                        // if (! (neighborX >= 0 && neighborX < chunkSize && ...)) の場合は renderFace = true のまま
                        // つまり、このif文を抜けて renderFace が false になるのは、
                        // 隣接するボクセルが「存在する」かつ「不透明」である場合のみ

                        if (renderFace)
                        {
                            // この面を描画する必要がある場合
                            // 各立方体の頂点を計算し、メッシュデータに追加
                            for (unsigned int baseIndex : cubeFaces[i])
                            {
                                Vertex baseVertex = baseCubeVertices[baseIndex];
                                Vertex newVertex = baseVertex;
                                // ボクセル位置にオフセットを適用し、間隔を考慮
                                newVertex.x += x * cubeSpacing;
                                newVertex.y += y * cubeSpacing;
                                newVertex.z += z * cubeSpacing;
                                meshData.vertices.push_back(newVertex);
                            }

                            // 各立方体のインデックスを計算し、メッシュデータに追加
                            // ここでのインデックスは、新たに meshData.vertices に追加された頂点に対するもの
                            for (int j = 0; j < 6; ++j)
                            {
                                // baseCubeIndices[j] を使う代わりに、
                                // 各面のために新しく追加された6つの頂点に対して0から5のインデックスを直接参照する
                                meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + j);
                            }
                            // currentVertexCount は次の面（もし描画されるなら）のために更新する
                            currentVertexCount = meshData.vertices.size();
                        }
                    }
                }
            }
        }
    }

    return meshData;
}