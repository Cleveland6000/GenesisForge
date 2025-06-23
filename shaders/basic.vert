#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 vertColor;

uniform mat4 model; // モデル行列
uniform mat4 view;  // ビュー行列
uniform mat4 projection; // 投影行列

void main()
{
    // 頂点位置に変換行列を適用
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vertColor = aColor;
}