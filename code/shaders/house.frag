#version 330 core

in vec3 FragPos;
in vec3 Normal;
in float MatType;

out vec4 FragColor;

uniform vec3 viewPos;
uniform float time;

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

void main() {
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // 光照
    vec3 lightDir = normalize(vec3(0.3, 1.0, 0.5));
    float diff = max(dot(norm, lightDir), 0.0);
    float ambient = 0.35;
    
    // 高光
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    
    vec3 baseColor;
    vec3 specular = vec3(0.0);
    
    // 材質類型判斷
    if (MatType < 0.5) {
        // 0 = 深棕色木材/柱子（第一層）
        baseColor = vec3(0.35, 0.2, 0.1);
    }
    else if (MatType < 1.5) {
        // 1 = 金色（第二、三層）
        baseColor = vec3(0.95, 0.8, 0.3);
        // 金色有較強的高光
        specular = vec3(1.0, 0.9, 0.5) * spec * 0.6;
    }
    else if (MatType < 2.5) {
        // 2 = 深灰色屋頂
        baseColor = vec3(0.12, 0.12, 0.15);
    }
    else if (MatType < 3.5) {
        // 3 = 淺棕色地板
        baseColor = vec3(0.55, 0.4, 0.25);
    }
    else {
        // 4 = 橘色欄杆
        baseColor = vec3(0.85, 0.45, 0.2);
        specular = vec3(0.3, 0.15, 0.05) * spec * 0.3;
    }
    
    // 計算最終顏色
    vec3 lighting = baseColor * (ambient + diff * 0.55);
    vec3 finalColor = lighting + specular;
    
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
    finalColor += fireworkAdd;

    FragColor = vec4(finalColor, 1.0);
}
