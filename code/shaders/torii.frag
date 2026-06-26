#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec3 LocalPos;
in float PartType;

uniform vec3 viewPos;

const vec3 moonDir = normalize(vec3(0.3, 0.8, 0.2));

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

// 簡單noise
float hash12(vec2 p) { 
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float noise2D(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    float a = hash12(i);
    float b = hash12(i + vec2(1.0, 0.0));
    float c = hash12(i + vec2(0.0, 1.0));
    float d = hash12(i + vec2(1.0, 1.0));
    vec2 u = f*f*(3.0-2.0*f);
    return mix(a, b, u.x) + (c-a)*u.y*(1.0-u.x) + (d-b)*u.x*u.y;
}

float fbm(vec2 p) {
    float v = 0.0;
    float a = 0.5;
    for(int i=0; i<4; i++) {
        v += a * noise2D(p);
        p *= 2.0;
        a *= 0.5;
    }
    return v;
}

void main() {
    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);
    vec3 L = moonDir;
    
    // 使用PartType判斷是否為笠木部分 (1.0 = 笠木)
    bool isKasagi = PartType > 0.5;
    
    // 黑色笠木 vs 葉子的亮紅色
    vec3 baseColor = isKasagi ? vec3(0.08, 0.08, 0.10) : vec3(0.93, 0.14, 0.045);
    
    // 木紋效果（subtle）
    float woodPattern = fbm(LocalPos.xy * 2.5 + LocalPos.z * 0.5);
    woodPattern += fbm(LocalPos.yz * 3.0) * 0.3;
    
    // 木紋變化
    float woodVar = smoothstep(0.3, 0.7, woodPattern);
    vec3 darkWood = baseColor * 0.75;
    vec3 lightWood = baseColor * 1.1;
    vec3 woodColor = mix(darkWood, lightWood, woodVar);
    
    // 老化效果
    float aging = fbm(FragPos.xz * 0.8) * 0.15;
    woodColor *= (1.0 - aging);
    
    // 月光照明
    float ambient = 0.008;
    float ndotl = max(dot(N, L), 0.0);
    
    // 高光（漆面反光）
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), 32.0) * 0.25;
    
    // 基礎顏色
    vec3 col = woodColor * (ambient + ndotl * 0.4) + vec3(spec);
    
    // 煙火動態光照
    vec3 fireworkLight = vec3(0.0);
    for (int i = 0; i < numLights && i < MAX_LIGHTS; ++i) {
        vec3 Ldir = lights[i].position - FragPos;
        float dist = length(Ldir);
        vec3 Lp = Ldir / max(dist, 1e-4);
        
        float att = calculateAttenuation(dist);
        float ndl = max(dot(N, Lp), 0.0);
        
        // 高光
        vec3 Hp = normalize(Lp + V);
        float specP = pow(max(dot(N, Hp), 0.0), 48.0) * 0.3;
        
        fireworkLight += (woodColor * ndl + vec3(specP)) 
                        * lights[i].color * lights[i].intensity * att * 0.08;
    }
    
    col += fireworkLight;
    
    // 稍微壓暗整體
    col *= 0.75;
    
    // 去飽和一點（夜晚效果）
    float luma = dot(col, vec3(0.299, 0.587, 0.114));
    col = mix(vec3(luma), col, 0.88);
    
    // Gamma校正
    col = pow(col, vec3(1.0/2.2));
    
    FragColor = vec4(col, 1.0);
}
