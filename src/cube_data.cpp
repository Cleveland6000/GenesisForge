#include "cube_data.hpp"

// 定数の定義（ここに実際のデータが格納される）
const float CUBE_VERTICES[] = {
    0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
    0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f,
    -0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 1.0f,
    0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
    -0.5f, 0.5f, 0.5f, 0.8f, 0.2f, 0.6f
};
const unsigned int CUBE_INDICES[] = {
    2, 1, 0, 0, 3, 2, 6, 5, 4, 4, 7, 6, 7, 3, 2, 2, 6, 7,
    0, 1, 5, 5, 4, 0, 3, 0, 4, 4, 7, 3, 2, 6, 5, 5, 1, 2
};

// サイズ情報も提供
const size_t CUBE_VERTICES_SIZE = sizeof(CUBE_VERTICES);
const size_t CUBE_INDICES_SIZE = sizeof(CUBE_INDICES);
const unsigned int CUBE_INDEX_COUNT = sizeof(CUBE_INDICES) / sizeof(CUBE_INDICES[0]);