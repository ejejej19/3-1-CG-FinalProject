#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in float HeightFactor;
in vec2 TexCoords;

uniform vec3 viewPos;
uniform sampler2D rockTexture;
uniform sampler2D snowTexture;
uniform float snowLine;      // 雪線高度 (世界座標 y)
uniform float snowBlend;     // 過渡寬度 (越大越柔)
uniform float noiseScale;    // 噪聲頻率 (越大越碎)
uniform float noiseAmp;      // 噪聲幅度 (起伏程度，單位：高度)
uniform float slopeStart;    // 開始不積雪的坡度門檻
uniform float slopeEnd;      // 完全不積雪的坡度門檻

struct Light {
    vec3 position;
    vec3 color;
    float intensity;
};

#define MAX_LIGHTS 100
uniform Light lights[MAX_LIGHTS];
uniform int numLights;

float calculateAttenuation(float distance) {
    float constant = 1.0;
    float linear = 0.10;
    float quadratic = 0.03;
    return 1.0 / (constant + linear * distance + quadratic * (distance * distance));
}

// 簡單噪聲函數
float hash(float n) { return fract(sin(n) * 43758.5453123); }

float hash12(vec2 p) {
    vec3 p3  = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float noise1D(float x) {
    float i = floor(x);
    float f = fract(x);
    float u = f * f * (3.0 - 2.0 * f);
    return mix(hash(i), hash(i + 1.0), u);
}

float noise2D(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    float a = hash12(i);
    float b = hash12(i + vec2(1.0, 0.0));
    float c = hash12(i + vec2(0.0, 1.0));
    float d = hash12(i + vec2(1.0, 1.0));
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
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
    vec3 up = vec3(0.0, 1.0, 0.0);
    
    // 光照方向（月光從上方偏右）
    vec3 L = normalize(vec3(0.3, 0.8, 0.2));
    // float diff = max(dot(N, L), 0.0);
    
    // 紋理變化
    vec2 texCoord = FragPos.xz * 0.02;
    float t = fbm(texCoord * 3.0);
    float t2 = fbm(texCoord * 12.0);
    
    // ===== 富士山風格配色（使用紋理）=====
    // 雪線起伏：用 xz 做 noise
    float n1 = noise2D(FragPos.xz * (noiseScale * 0.75));     // 低頻：大形狀
    float n2 = noise2D(FragPos.xz * (noiseScale * 0.45));  // 高頻：細破碎
    float n  = mix(n1, n2, 0.55);                         // 混合
    float snowOffset = (n * 2.0 - 1.0) * noiseAmp; // -noiseAmp..+noiseAmp
    
    // 頂部約 1/10 雪量 + 不規則雪線
    float baseLine = 0.90;
    float irregular = (n * 2.0 - 1.0) * 0.14;   // 雪線起伏幅度
    float line = baseLine + irregular;

    float edgeJitter = (noise2D(FragPos.xz * (noiseScale * 2.0)) * 2.0 - 1.0) * 0.03;
    line += edgeJitter; // 只加很小：讓邊緣稍微破，但不會碎

    float blend = 0.10;                         // 雪線過渡寬度（越大越柔）
    float snowAmt = smoothstep(line - blend, line + blend, HeightFactor); 
    snowAmt = pow(snowAmt, 0.65);

    // smoothstep 做柔和過渡
    // float snowMask = smoothstep(-snowBlend, snowBlend, snowT);  

    // 坡度抑制：陡坡比較不積雪
    float slope = 1.0 - dot(N, up); // 0 平坦 -> 1 垂直
    float slopeMask = 1.0 - smoothstep(slopeStart, slopeEnd, slope);

    // 最終雪量
    snowAmt *= slopeMask;

    // ===== 雪的微起伏法線（假 bump）=====
    // 用 fbm 在 xz 平面做一個「雪高度場」，用有限差分估 gradient 來造法線
    float snowFreq = 0.18;      // 起伏的「大小」(越小越大塊，越大越碎)
    float snowAmpN = 2.2;      // 起伏造成的法線強度（越大陰影越明顯）
    float e = 0.8;              // 差分距離（世界座標，太小會抖/太大會糊）

    float h0  = fbm(FragPos.xz * snowFreq);
    float hx  = fbm((FragPos.xz + vec2(e, 0.0)) * snowFreq);
    float hz  = fbm((FragPos.xz + vec2(0.0, e)) * snowFreq);

    // dh/dx, dh/dz
    float dhdx = (hx - h0) / e;
    float dhdz = (hz - h0) / e;

    // 雪的微法線：y 給 1，x/z 用斜率（乘上 snowAmpN 控制陰影強度）
    vec3 N_snow = normalize(vec3(-dhdx * snowAmpN, 1.0, -dhdz * snowAmpN));

    // 只在雪區域混入微法線（snowAmt 越大越像雪的法線）
    vec3 N_use = normalize(mix(N, N_snow, clamp(snowAmt, 0.0, 1.0)));

    // ===== 岩面的微起伏法線（假 bump）=====
    // 做法跟雪一樣：用另一組 fbm 當岩面高度場，算 gradient 變成法線
    float rockFreq = 0.35;      // 岩面細節頻率（越大越碎）
    float rockAmpN = 4.5;       // 岩面法線強度（越大凹凸越明顯）
    float er = 0.8;             // 差分距離

    float r0 = fbm(FragPos.xz * rockFreq);
    float rx = fbm((FragPos.xz + vec2(er, 0.0)) * rockFreq);
    float rz = fbm((FragPos.xz + vec2(0.0, er)) * rockFreq);

    float drdx = (rx - r0) / er;
    float drdz = (rz - r0) / er;

    vec3 N_rock = normalize(vec3(-drdx * rockAmpN, 1.0, -drdz * rockAmpN));

    // 把 N_use 改成：雪用 N_snow、非雪用 N_rock、中間自然混合
    N_use = normalize(mix(N_rock, N_snow, clamp(snowAmt, 0.0, 1.0)));

    // 採樣岩石紋理
    vec3 rockTex = texture(rockTexture, TexCoords).rgb;

    // 程序化岩石細節 - 增加岩石質感
    float rockNoise = fbm(texCoord * 8.0);
    float rockDetail = fbm(texCoord * 25.0);
    float rockCrack = fbm(texCoord * 50.0);  // 裂縫細節
    
    // 岩石基底顏色（深墨綠色調）
    vec3 rockDark = vec3(0.004, 0.01, 0.007);   // 極深墨綠
    vec3 rockMid = vec3(0.01, 0.024, 0.015);    // 深墨綠
    vec3 rockLight = vec3(0.02, 0.045, 0.028);  // 中墨綠
    
    // 混合岩石顏色產生自然變化
    vec3 rockBase = mix(rockDark, rockMid, rockNoise);
    rockBase = mix(rockBase, rockLight, rockDetail * 0.5);
    
    // 加入紋理貼圖的細節
    rockBase *= (0.55 + 0.35 * rockTex.r);
    
    // 裂縫和凹陷（讓岩石更有立體感）
    rockBase *= (0.85 + 0.3 * rockCrack);
    
    // 採樣雪紋理（用沙地紋理模擬雪的質感）
    vec3 snowTex = texture(snowTexture, TexCoords * 0.5).rgb;
    // 讓雪更白更亮
    snowTex = mix(vec3(0.90, 0.92, 0.98), snowTex * 0.2 + vec3(0.8), 0.4);
    
    // 混合岩石和雪紋理（使用程序化岩石）
    // vec3 baseCol = mix(rockBase, snowTex, snowAmt);

    // 融雪/髒雪帶：在 snowAmt 約 0.25~0.75 時最明顯
    float slush = smoothstep(0.20, 0.55, snowAmt) * (1.0 - smoothstep(0.55, 0.90, snowAmt));

    // 髒雪顏色（灰白、帶一點冷色）
    vec3 slushCol = mix(rockBase, vec3(0.72, 0.74, 0.78), 0.65);

    // 三段混色：先進入髒雪，再進入白雪
    vec3 col1 = mix(rockBase, slushCol, slush);
    vec3 baseCol = mix(col1, snowTex, snowAmt);

    // 加入紋理細節（岩石區域有更多細節）
    float detailStrength = mix(0.3, 0.05, snowAmt);
    baseCol *= (1.0 - detailStrength + detailStrength * 2.0 * t2);
    
    // 光照
    float diff = max(dot(N_use, L), 0.0);
    // cavity 越大代表越像凹處
    float cavitySnow = smoothstep(0.55, 0.25, h0);  // h0 低的地方偏凹
    float snowAO = 1.0 - 0.18 * cavitySnow * snowAmt; // 0.18 可調：越大凹處越黑
    float cavityRock = smoothstep(0.55, 0.25, r0);
    float rockAO = 1.0 - 0.12 * cavityRock * (1.0 - snowAmt);
    float ambient = 0.1;  // 稍微提高環境光讓雪更亮
    float shade = ambient + (1.0 - ambient) * diff;
    
    // 坡度暗化（陡峭處較暗）- 對雪的影響較小
    float slopeFactor = dot(N, vec3(0.0, 1.0, 0.0));
    float slopeShade = 0.6 + 0.4 * slopeFactor;
    shade *= mix(slopeShade, 0.9, snowAmt);  // 雪面受坡度影響較小
    
    vec3 col = baseCol * shade * snowAO * rockAO;
    
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
    
    // ===== Add firework lighting (added) =====
    vec3 fireworkAdd = vec3(0.0);

    for (int i = 0; i < numLights && i < MAX_LIGHTS; ++i) {
        // 水面是用 intensity > 100 判斷煙火
        // if (lights[i].intensity <= 100.0) continue;

        vec3 Ldir = lights[i].position - FragPos;
        float dist = length(Ldir);
        vec3 Lp = Ldir / max(dist, 1e-4);

        float att = calculateAttenuation(dist);

        // diffuse：不改原本材質
        float ndl = max(dot(N_use, Lp), 0.0);
        float Boost = mix(0.65, 1.15, 1.0);
        vec3 diffAdd = baseCol * ndl * Boost;

        // spec：讓煙火在葉子上有一點亮點（不要太塑膠）
        vec3 Hp = normalize(Lp + V);
        float specP = pow(max(dot(N_use, Hp), 0.0), mix(24.0, 48.0, 1.0));
        vec3 specAdd = vec3(specP) * mix(0.03, 0.07, 1.0);

        // 疊加：強度縮放（這個係數是你最主要要調的地方）
        fireworkAdd += (diffAdd + specAdd) * lights[i].color * lights[i].intensity * att * 0.02;
    }

    // 加到原本 col（只加，不影響原本月光/材質）
    col += fireworkAdd;

    // Gamma 校正
    col = pow(col, vec3(1.0 / 2.2));
    
    FragColor = vec4(col, alpha);
}
