#version 330 core
out vec4 FragColor;

in vec3 ourColor; // 頂点シェーダーから受け取る色 (今回は使用しない)
in vec2 TexCoord; // 頂点シェーダーから受け取るテクスチャ座標
in vec3 Normal; // ワールド空間の法線を受け取る

uniform sampler2D ourTexture; // テクスチャサンプラー
uniform vec3 lightDir; // 光源の方向 (正規化されていると仮定)
uniform float ambientStrength; // 環境光の強さ (0.0から1.0)

void main()
{
    vec3 texColor = texture(ourTexture, TexCoord).rgb; // テクスチャの色を取得

    // 環境光 (Ambient Light)
    vec3 ambient = ambientStrength * texColor;

    // 拡散反射光 (Diffuse Light)
    // 法線と光源方向の内積を計算。面が光源と反対を向いている場合は0にする
    float diff = max(dot(Normal, -lightDir), 0.0); // 光源方向を反転 (-lightDir)
    vec3 diffuse = diff * texColor; // テクスチャの色に拡散反射光を適用

    // 最終的な色を計算 (環境光 + 拡散反射光)
    FragColor = vec4(ambient + diffuse, 1.0);
}