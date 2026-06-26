#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in float IsLight;

uniform vec3 viewPos;
uniform vec3 lanternColor;

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

void main() {
    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);
    vec3 L = moonDir;
    
    // 判斷材質類型：0=紅色柱子, 1=發光燈罩, 2=黑色框架
    float matType = IsLight;
    bool isLanternLight = (matType > 0.5 && matType < 1.5);
    bool isFrame = (matType > 1.5);
    
    vec3 baseColor;
    vec3 col;
    
    if (isLanternLight) {
        // 發光燈罩 - 暖黃色發光
        baseColor = lanternColor;
        
        // 強烈自發光
        float emissive = 1.8;
        col = baseColor * emissive;
        
        // 輕微受月光影響
        float ambient = 0.3;
        float ndotl = max(dot(N, L), 0.0) * 0.2;
        col += baseColor * (ambient + ndotl) * 0.2;
        
    } else if (isFrame) {
        // 黑色框架和屋頂
        baseColor = vec3(0.08, 0.08, 0.10);
        
        // 月光照明
        float ambient = 0.005;
        float ndotl = max(dot(N, L), 0.0);
        
        col = baseColor * (ambient + ndotl * 0.4);
        
        // 輕微高光
        vec3 H = normalize(L + V);
        float spec = pow(max(dot(N, H), 0.0), 32.0) * 0.15;
        col += vec3(spec);
        
    } else {
        // 紅色木質柱子 - 使用葉子的亮紅色
        baseColor = vec3(0.93, 0.14, 0.045);
        
        // 月光照明
        float ambient = 0.01;
        float ndotl = max(dot(N, L), 0.0);
        
        col = baseColor * (ambient + ndotl * 0.5);
        
        // 高光
        vec3 H = normalize(L + V);
        float spec = pow(max(dot(N, H), 0.0), 24.0) * 0.2;
        col += vec3(spec);
    }
    
    // 煙火動態光照（對所有部分）
    vec3 fireworkLight = vec3(0.0);
    for (int i = 0; i < numLights && i < MAX_LIGHTS; ++i) {
        vec3 Ldir = lights[i].position - FragPos;
        float dist = length(Ldir);
        vec3 Lp = Ldir / max(dist, 1e-4);
        
        float att = calculateAttenuation(dist);
        float ndl = max(dot(N, Lp), 0.0);
        
        vec3 diffAdd = baseColor * ndl;
        
        fireworkLight += diffAdd * lights[i].color * lights[i].intensity * att * 0.05;
    }
    
    col += fireworkLight;
    
    // 稍微壓暗整體（除了發光部分）
    if (!isLanternLight) {
        col *= 0.75;
    }
    
    // Gamma校正
    col = pow(col, vec3(1.0/2.2));
    
    FragColor = vec4(col, 1.0);
}
