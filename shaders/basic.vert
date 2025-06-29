#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor; // 既存の色属性 (使用しない場合でも互換性のため残す)
layout (location = 2) in vec2 aTexCoord; // テクスチャ座標属性
layout (location = 3) in vec3 aNormal; // 法線属性を追加

out vec3 ourColor; // (使用しない場合でも互換性のため残す)
out vec2 TexCoord; // フラグメントシェーダーへ渡すテクスチャ座標
out vec3 Normal; // ワールド空間の法線を出力

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix; // 法線を変換するための行列 (モデル行列の逆転置)

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    ourColor = aColor; // 色もそのまま渡す
    TexCoord = aTexCoord; // テクスチャ座標を渡す
    // 法線をワールド空間に変換し、正規化して出力
    Normal = normalize(normalMatrix * aNormal);
}