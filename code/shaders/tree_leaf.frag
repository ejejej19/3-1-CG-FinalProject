#version 330 core
out vec4 FragColor;

in vec3 LeafColor;
in vec2 TexCoord;
in float LeafRandom;  // 每片葉子的隨機值

// 簡單的噪聲函數
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

void main() {
    vec2 center = vec2(0.5, 0.5);
    vec2 uv = TexCoord - center;
    
    // 使用葉子隨機值來創造不同形狀
    float seed = LeafRandom * 100.0;
    
    // 極座標
    float angle = atan(uv.y, uv.x);
    float dist = length(uv);
    
    // 不規則邊緣 - 多個頻率的變形
    float edgeNoise = 0.0;
    edgeNoise += sin(angle * 3.0 + seed) * 0.15;           // 低頻大變形
    edgeNoise += sin(angle * 5.0 + seed * 1.3) * 0.1;      // 中頻
    edgeNoise += sin(angle * 8.0 + seed * 2.1) * 0.06;     // 高頻細節
    edgeNoise += noise(vec2(angle * 2.0, seed)) * 0.12;    // 噪聲細節
    
    // 基礎形狀 - 略橢圓且不規則
    float stretch = 1.0 + sin(angle * 2.0 + seed) * 0.3;   // 橢圓變形
    float baseRadius = 0.35 + edgeNoise;
    
    // 距離邊緣的程度
    float edgeDist = dist * stretch - baseRadius;
    
    // 柔和邊緣
    float alpha = 1.0 - smoothstep(-0.08, 0.05, edgeDist);
    
    // 葉脈效果 - 增加自然感
    float vein = abs(sin(angle * 4.0 + seed) * cos(dist * 15.0));
    vein = smoothstep(0.7, 1.0, vein) * 0.15;
    
    // 添加內部紋理變化
    float innerNoise = noise(TexCoord * 8.0 + seed) * 0.2;
    alpha *= (0.85 + innerNoise - vein);
    
    // 隨機的透明度變化
    alpha *= 0.8 + hash(vec2(seed, seed * 0.7)) * 0.2;
    
    if (alpha < 0.05) discard;
    
    // 顏色微調 - 邊緣略暗
    vec3 finalColor = LeafColor * (0.9 + 0.1 * (1.0 - dist * 2.0));
    
    FragColor = vec4(finalColor, alpha);
}
