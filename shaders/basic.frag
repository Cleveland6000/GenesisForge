#version 330 core
out vec4 FragColor;

in vec3 ourColor; // 頂点シェーダーから受け取る色 (今回は使用しない)
in vec2 TexCoord; // 頂点シェーダーから受け取るテクスチャ座標

uniform sampler2D ourTexture; // テクスチャサンプラー

void main()
{
    // テクスチャの色のみを使用する
    FragColor = texture(ourTexture, TexCoord);
}