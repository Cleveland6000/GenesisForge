#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D fontAtlas;
uniform vec3 textColor;
float pxRange = 4.0; // ここはスクリプトで確認した通り4.0でOK

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

void main()
{
    vec3 sdf_color = texture(fontAtlas, TexCoord).rgb;
    float dist = median(sdf_color.r, sdf_color.g, sdf_color.b);
    dist = dist + 0.25;
    // SDF距離を画面ピクセル距離に変換
    // fwidthを適用する前に、distを0.0-1.0の範囲で処理し、
    // smoothstepの閾値をdistに対して動的に調整する
    // これにより、様々なスケールでアンチエイリアシングがより自然になります
    float edge_width = fwidth(dist); // distの勾配を計算 (画面空間でのテクセル間の距離の変化)

    // smoothstepの範囲は、SDFの中央値(0.5)を中心に、edge_widthの半分で広げる
    
    float outline_width = fwidth(dist); 
    float alpha = smoothstep(0.5 - outline_width, 0.5 + outline_width, dist);
    // 最終的な色
    FragColor = vec4(textColor, alpha);
}