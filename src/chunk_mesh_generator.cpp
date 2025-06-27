// src/chunk_mesh_generator.cpp
#include "chunk_mesh_generator.hpp"
#include <glm/glm.hpp> // glm::vec3 を使うためにインクルード

// 単一立方体の頂点データとインデックスデータ
// (以前の generateCubeMesh() で使っていた定数データは削除)

// generateCubeMesh() の実装は削除する

// !!!ここが変更点!!! generateMesh() の実装を追加
ChunkMeshData ChunkMeshGenerator::generateMesh(const Chunk &chunk, float cubeSpacing)
{
    ChunkMeshData meshData;
    int chunkSize = chunk.getSize();

    // 各立方体の基本頂点とインデックス
    // これらはオフセットされて各ボクセル位置に配置されます
    // 位置 (x,y,z) と 色 (r,g,b)
    const std::vector<Vertex> baseCubeVertices = {
        {-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f}, // 0: Back-bottom-left (黒)
        {0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f},  // 1: Back-bottom-right (赤)
        {0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f},   // 2: Back-top-right (緑)
        {-0.5f, 0.5f, -0.5f, 0.0f, 0.0f, 1.0f},  // 3: Back-top-left (青)
        // Front face (Z-positive)
        {0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f},   // 4: Front-top-right (黄)
        {0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 1.0f},  // 5: Front-bottom-right (シアン)
        {-0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 1.0f}, // 6: Front-bottom-left (マゼンタ)
        {-0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f}   // 7: Front-top-left (グレー)
    };

    // 立方体のインデックスデータ (6面 x 2三角形/面 x 3インデックス/三角形 = 36インデックス)
    const std::vector<unsigned int> baseCubeIndices = {
        // 背面
        0, 2, 1, 0, 3, 2,
        // 前面
        6, 5, 4, 4, 7, 6,
        // 右面
        1, 4, 5, 1, 2, 4,
        // 左面
        6, 3, 0, 6, 7, 3,
        // 上面
        3, 4, 2, 3, 7, 4,
        // 下面
        6, 1, 5, 6, 0, 1};

    // チャンク内の各ボクセルを走査
    for (int x = 0; x < chunkSize; ++x)
    {
        for (int y = 0; y < chunkSize; ++y)
        {
            for (int z = 0; z < chunkSize; ++z)
            {
                if (chunk.getVoxel(x, y, z))
                { // ボクセルが存在する場合
                    // 現在のメッシュの頂点数（インデックスのオフセットに使う）
                    size_t currentVertexCount = meshData.vertices.size();

                    // 各立方体の頂点を計算し、メッシュデータに追加
                    for (const auto &baseVertex : baseCubeVertices)
                    {
                        Vertex newVertex = baseVertex;
                        // ボクセル位置にオフセットを適用し、間隔を考慮
                        newVertex.x += x * cubeSpacing;
                        newVertex.y += y * cubeSpacing;
                        newVertex.z += z * cubeSpacing;
                        meshData.vertices.push_back(newVertex);
                    }

                    // 各立方体のインデックスを計算し、メッシュデータに追加
                    // 現在の頂点数分オフセットを適用する
                    for (unsigned int baseIndex : baseCubeIndices)
                    {
                        meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + baseIndex);
                    }
                }
            }
        }
    }

    return meshData;
}
