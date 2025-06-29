#version 330 core
out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;
in vec3 Normal;
in float AO; // <--- 頂点シェーダーから受け取るAO値

uniform sampler2D ourTexture;
uniform vec3 lightDir;
uniform float ambientStrength;

void main()
{
    vec3 texColor = texture(ourTexture, TexCoord).rgb;

    // AO値を0-1の範囲に正規化し、環境光に適用
    // AO値が0の場合は最も暗く、3の場合は最も明るい (1.0) になるように調整
    // ここでのAOはブロックされていない度合いを示すので、値が大きいほど明るい。
    // そのため、乗算で適用するのが自然。
    float ambientOcclusionFactor = (AO / 3.0); // 0/3, 1/3, 2/3, 3/3 = 0.0, ~0.33, ~0.66, 1.0

    // 環境光 (Ambient Light) にAOを適用
    vec3 ambient = ambientStrength * texColor * ambientOcclusionFactor;

    // 拡散反射光 (Diffuse Light)
    float diff = max(dot(Normal, -lightDir), 0.0);
    vec3 diffuse = diff * texColor;

    // 最終的な色を計算 (環境光 + 拡散反射光)
    FragColor = vec4(ambient + diffuse, 1.0);
}