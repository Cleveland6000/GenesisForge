#version 330 core
out vec4 FragColor; // フラグメントシェーダーの最終出力

in vec3 vertColor; // 頂点シェーダーから受け取る色 (名前が一致しているか？)

void main()
{
    FragColor = vec4(vertColor, 1.0f); // 受け取った色を使用
}