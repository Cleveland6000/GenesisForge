#ifndef CUBE_DATA_HPP
#define CUBE_DATA_HPP

#include <vector>

// 立方体の頂点データとインデックスデータを外部からアクセスできるように宣言
// extern を使うことで、他のファイルでこの定数にアクセスできる
extern const float CUBE_VERTICES[];
extern const unsigned int CUBE_INDICES[];
extern const size_t CUBE_VERTICES_SIZE; // 配列のバイトサイズ
extern const size_t CUBE_INDICES_SIZE;  // 配列のバイトサイズ
extern const unsigned int CUBE_INDEX_COUNT; // インデックスの数 (36)

#endif // CUBE_DATA_HPP