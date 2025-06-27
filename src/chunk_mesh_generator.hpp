// src/chunk_mesh_generator.hpp
#ifndef CHUNK_MESH_GENERATOR_HPP
#define CHUNK_MESH_GENERATOR_HPP

#include <vector>
#include <cstdint> // for size_t
#include "chunk/chunk.hpp" // Chunk クラスの定義をインクルード

// 頂点構造体: 位置と色を含む
struct Vertex {
    float x, y, z;      // 位置
    float r, g, b;      // 色
    // 必要に応じて、法線、テクスチャ座標などをここに追加
};

// チャンクの純粋なメッシュデータを保持する構造体
// OpenGLのバッファ（VAO/VBO/EBO）は含まず、純粋なCPU側のデータ
struct ChunkMeshData {
    std::vector<Vertex> vertices;          // 頂点データの配列
    std::vector<unsigned int> indices;     // インデックスデータの配列
};

class ChunkMeshGenerator {
public:
    // !!!ここが変更点!!! generateCubeMesh() を削除し、generateMesh() を追加
    // static ChunkMeshData generateCubeMesh(); // 削除する

    // Chunkオブジェクトからメッシュデータを生成する
    // cubeSpacing: 各ボクセルの間に適用する間隔（スケール）
    static ChunkMeshData generateMesh(const Chunk& chunk, float cubeSpacing = 1.0f); 
};

#endif // CHUNK_MESH_GENERATOR_HPP