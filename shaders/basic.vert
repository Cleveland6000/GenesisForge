#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor; // 既存の色属性
layout (location = 2) in vec2 aTexCoord; // 新しいテクスチャ座標属性

out vec3 ourColor;
out vec2 TexCoord; // フラグメントシェーダーへ渡すテクスチャ座標

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    ourColor = aColor; // 色もそのまま渡す
    TexCoord = aTexCoord; // テクスチャ座標を渡す
}