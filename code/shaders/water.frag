#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;
in vec4 ClipSpacePos;
in vec4 ReflClipPos;  // 反射相機投影後的位置

uniform vec3 viewPos;
uniform vec3 waterColor;
uniform float waterAlpha;
uniform float time;
uniform float lakeRadius;
uniform sampler2D waterTexture; // 模擬水面波紋
uniform sampler2D normalTexture;
uniform sampler2D reflectionTexture;

// 光源結構體
struct Light {
    vec3 position;
    vec3 color;
    float intensity;
};

#define MAX_LIGHTS 100
uniform Light lights[MAX_LIGHTS];
uniform int numLights;

// 光線衰減函數
float calculateAttenuation(float distance) {
    float constant = 1.0;
    float linear = 0.1; // 衰減 -> 避免過暗看不出是水
    float quadratic = 0.03;
    return 1.0 / (constant + linear * distance + quadratic * (distance * distance));
}

// 用高度場 h(x,z) 的偏導數做波紋法線
vec3 waveNormal(vec3 baseN, vec3 p, float t) {
    float x = p.x;
    float z = p.z;

    // 兩組不同方向 / 頻率的波
    float f1 = 0.28, s1 = 1.8, a1 = 0.38;
    float f2 = 0.15, s2 = 1.0, a2 = 0.23;

    float h1 = sin(x*f1 + t*s1) + sin(z*f1 - t*s1);
    float h2 = sin((x+z)*f2 + t*s2);

    // dh/dx, dh/dz
    float dhdx = a1 * (cos(x*f1 + t*s1)*f1 + 0.0) + a2 * (cos((x+z)*f2 + t*s2)*f2);
    float dhdz = a1 * (0.0 + cos(z*f1 - t*s1)*f1) + a2 * (cos((x+z)*f2 + t*s2)*f2);

    // baseN 通常是 (0,1,0)，把波紋斜率加進去
    vec3 n = normalize(vec3(-dhdx, 1.0, -dhdz));
    // 混回 baseN，避免太皺
    return normalize(mix(baseN, n, 0.80));
}

void main() {
    // 採樣法線貼圖 (如果有的話)
    vec3 N = waveNormal(normalize(Normal), FragPos, time); // normalize(Normal);
    vec3 V  = normalize(viewPos - FragPos);

    // Fresnel: 越斜看越像鏡面（更像水）
    float ndv = clamp(dot(N, V), 0.0, 1.0);
    float fresnel = pow(1.0 - ndv, 5.0);
    fresnel = 0.05 + 0.95 * fresnel;
    float fresnelMax = 0.90;  // 提高反射上限，讓倒影更明顯
    fresnel = min(fresnel, fresnelMax);
    float grazingDamp = 0.75 + 0.25 * ndv;   // 提高基礎反射
    fresnel *= grazingDamp;

    // 基礎水色：做深淺，往下看更深更暗，斜看更亮
    vec3 shallow = waterColor * vec3(0.95, 1.0, 1.0); // 淺水稍亮
    vec3 deep    = waterColor * vec3(0.15, 0.25, 0.42); // 深水偏暗
    vec3 baseColor = mix(shallow, deep, ndv);
    vec3 tint = texture(waterTexture, TexCoords).rgb; 
    baseColor *= mix(vec3(1.0), tint, 0.6); 
    
    // 光照計算
    vec3 ambient = vec3(0.006);
    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);
    
    bool hasFirework = false;
    for (int i = 0; i < numLights && i < MAX_LIGHTS; ++i) {
        if (lights[i].intensity > 100.0) {
            hasFirework = true;
            break;
        }
    }

    for (int i = 0; i < numLights && i < MAX_LIGHTS; ++i) {
        vec3 Ldir = lights[i].position - FragPos;
        float distance = length(Ldir);
        float attenuation = calculateAttenuation(distance);
        
        vec3 L = normalize(Ldir);
        vec3 H = normalize(L + V);

        float ambK = hasFirework ? 0.01 : 0.08;
        float difK = hasFirework ? 0.05 : 0.35;

        // 環境光 (提升亮度)
        ambient += ambK * lights[i].color * lights[i].intensity * attenuation;
        
        // 漫反射 (提升亮度)
        float diff = max(dot(N, L), 0.0);
        diffuse += difK * diff * lights[i].color * lights[i].intensity * attenuation;
        
        // 鏡面反射 (水面高光)
        float spec = pow(max(dot(N, H), 0.0), 192.0);
        specular += 2.6 * spec * lights[i].color * lights[i].intensity * attenuation;
    }
    
    // 煙花爆炸時的特殊反射效果
    vec3 fireworkReflection = vec3(0.0);
    for (int i = 0; i < numLights && i < MAX_LIGHTS; ++i) {
        // 只有強度較高的光源（煙花爆炸）才產生明顯的水面反射
        if (lights[i].intensity > 130.0) {
            float distance = length(lights[i].position - FragPos);
            float reflectionIntensity = lights[i].intensity / (distance * distance + 1.0);
            
            // 反射點的位置 (光源在水面下的鏡像)
            vec3 reflectedLightPos = lights[i].position;
            reflectedLightPos.y = -reflectedLightPos.y; // 假設水面在 y=0
            
            vec3 toReflection = normalize(reflectedLightPos - FragPos);
            float reflectionStrength = max(dot(V, toReflection), 0.0);
            reflectionStrength = pow(reflectionStrength, 2.0);
            
            fireworkReflection += lights[i].color * reflectionIntensity * reflectionStrength * 0.01;
        }
    }
    
    // ===== 精確反射貼圖採樣 =====
    // 使用反射相機的投影座標來計算正確的 UV
    vec2 reflNDC = ReflClipPos.xy / ReflClipPos.w;  // 透視除法 [-1,1]
    vec2 reflUV = reflNDC * 0.5 + 0.5;  // 轉換到 [0,1]
    
    // 加入微小的法線扭曲，讓倒影有水波效果但保持清晰
    vec2 distortion = N.xz * 0.02;  // 很小的扭曲
    reflUV += distortion;
    reflUV = clamp(reflUV, 0.002, 0.998);
    
    // 採樣反射貼圖
    vec3 reflCol = texture(reflectionTexture, reflUV).rgb;
    // 壓縮反射貼圖的亮部，避免爆炸瞬間 reflTex 亮到一片白
    float lum = dot(reflCol, vec3(0.2126, 0.7152, 0.0722));
    float knee = hasFirework ? 1.2 : 999.0;      // 只有煙火時啟用
    if (lum > knee) {
        reflCol *= (knee / lum);
    }

    // 根據距離和角度調整反射清晰度
    float distFromCenter = length(FragPos.xz);
    float edgeFade = 1.0 - smoothstep(lakeRadius * 0.6, lakeRadius * 0.95, distFromCenter);
    
    // 保持煙火的原始亮度
    float envRefl = hasFirework ? 0.25 : 0.15;   // 沒煙火時大幅壓暗天空反射
    reflCol *= envRefl * edgeFade;

    // 最終顏色組合: 夜色 + Fresnel
    vec3 waterLit = (ambient + diffuse) * baseColor;
    // vec3 result = waterLit + specular * 0.15;
    float specScale = hasFirework ? 0.005 : 0.15;  // 爆炸時大幅壓高光
    vec3 result = waterLit + specular * specScale;

    // 反射混合 - 根據 Fresnel 和距離，大幅提高反射強度
    // float reflStrength = clamp(fresnel * 2.0, 0.3, 0.95);  // 基礎反射強度
    float maxRefl = hasFirework ? 0.95 : 0.45;
    float reflStrength = clamp(fresnel * 1.2, 0.02, maxRefl);
    result = mix(result, reflCol, reflStrength * edgeFade);
    
    // 額外加入明亮反射（讓煙火倒影更亮）
    // result += reflCol * 0.35 * edgeFade;
    
    // 加入一點環境反射顏色 (模擬天空反射)
    // vec3 skyReflection = vec3(0.15, 0.25, 0.4); // 更亮的夜空藍色
    // result = mix(result, skyReflection, fresnel * 0.3);
    
    // 透明度：基礎透明度 + Fresnel 調整（更透明）
    result = pow(result, vec3(1.0 / 2.2));
    float alpha = mix(waterAlpha, 0.75, fresnel);  // 降低最大不透明度
    
    FragColor = vec4(result, alpha);
}
