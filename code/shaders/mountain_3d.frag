#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in float HeightFactor;

uniform vec3 viewPos;

// 簡單噪聲函數
float hash(float n) { return fract(sin(n) * 43758.5453123); }
float noise1D(float x) {
    float i = floor(x);
    float f = fract(x);
    float u = f * f * (3.0 - 2.0 * f);
    return mix(hash(i), hash(i + 1.0), u);
}

float hash21(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float noise2(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    float a = hash21(i + vec2(0, 0));
    float b = hash21(i + vec2(1, 0));
    float c = hash21(i + vec2(0, 1));
    float d = hash21(i + vec2(1, 1));
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float fbm(vec2 p) {
    float v = 0.0;
    float a = 0.5;
    for (int i = 0; i < 5; i++) {
        v += a * noise2(p);
        p *= 2.0;
        a *= 0.5;
    }
    return v;
}

void main() {
    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);
    
    // 光照方向（月光從上方偏右）
    vec3 L = normalize(vec3(0.3, 0.8, 0.2));
    float diff = max(dot(N, L), 0.0);
    
    // 紋理變化
    vec2 texCoord = FragPos.xz * 0.02;
    float t = fbm(texCoord * 3.0);
    float t2 = fbm(texCoord * 12.0);
    
    // 基礎顏色：岩石
    vec3 rockA = vec3(0.12, 0.14, 0.18);
    vec3 rockB = vec3(0.22, 0.24, 0.28);
    vec3 rock = mix(rockA, rockB, t);
    
    // 雪線：高處有雪
    vec3 snow = vec3(0.75, 0.78, 0.82);
    float snowLine = 0.55 + 0.1 * (t - 0.5);
    float snowAmt = smoothstep(snowLine, 0.85, HeightFactor);
    vec3 baseCol = mix(rock, snow, snowAmt);
    
    // 加入紋理細節
    baseCol *= (0.85 + 0.3 * (t2 - 0.5));
    
    // 光照
    float ambient = 0.25;
    float shade = ambient + (1.0 - ambient) * diff;
    
    // 坡度暗化（陡峭處較暗）
    float slopeFactor = dot(N, vec3(0.0, 1.0, 0.0));
    shade *= 0.7 + 0.3 * slopeFactor;
    
    vec3 col = baseCol * shade;
    
    // 霧效果（遠處融入天空）
    float dist = length(viewPos - FragPos);
    float fogStart = 100.0;
    float fogEnd = 350.0;
    float fog = smoothstep(fogStart, fogEnd, dist);
    vec3 fogCol = vec3(0.08, 0.10, 0.15);
    col = mix(col, fogCol, fog);
    
    // 邊緣淡出（高度接近 0 處淡出）
    float heightFade = smoothstep(0.0, 0.15, HeightFactor);
    float alpha = heightFade * (1.0 - fog * 0.8);
    
    // 如果高度太低就丟棄
    if (HeightFactor < 0.01) discard;
    
    // Gamma 校正
    col = pow(col, vec3(1.0 / 2.2));
    
    FragColor = vec4(col, alpha);
}
