#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 viewPos;

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
    float linear = 0.09;
    float quadratic = 0.032;
    return 1.0 / (constant + linear * distance + quadratic * (distance * distance));
}

// 簡單噪聲
float hash12(vec2 p){ 
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float noise2D(vec2 p){
    vec2 i = floor(p);
    vec2 f = fract(p);
    float a = hash12(i);
    float b = hash12(i + vec2(1.0, 0.0));
    float c = hash12(i + vec2(0.0, 1.0));
    float d = hash12(i + vec2(1.0, 1.0));
    vec2 u = f*f*(3.0-2.0*f);
    return mix(a, b, u.x) + (c-a)*u.y*(1.0-u.x) + (d-b)*u.x*u.y;
}

float fbm(vec2 p){
    float v = 0.0;
    float a = 0.5;
    for(int i=0; i<4; i++){
        v += a * noise2D(p);
        p *= 2.0;
        a *= 0.5;
    }
    return v;
}

void main() {
    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);
    
    // 樹幹深棕色
    vec3 barkBase = vec3(0.35, 0.22, 0.12);   // 深褐色
    vec3 barkTint = vec3(0.45, 0.30, 0.18);   // 稍亮的褐色
    
    // 使用位置添加紋理變化
    float noise1 = fbm(FragPos.xz * 2.0 + FragPos.y * 0.5);
    float noise2 = fbm(FragPos.xz * 5.0);
    
    vec3 barkColor = mix(barkBase, barkTint, noise1);
    barkColor *= (0.85 + 0.3 * noise2);  // 添加明暗變化
    
    // 環境光
    vec3 ambient = barkColor * 0.4;
    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);
    
    // 基本月光方向
    vec3 moonDir = normalize(vec3(0.3, 0.8, 0.2));
    float moonDiff = max(dot(N, moonDir), 0.0);
    diffuse += barkColor * moonDiff * 0.5;
    
    // 動態光源
    for (int i = 0; i < numLights && i < MAX_LIGHTS; i++) {
        vec3 L = normalize(lights[i].position - FragPos);
        vec3 H = normalize(L + V);
        
        float distance = length(lights[i].position - FragPos);
        float attenuation = calculateAttenuation(distance);
        
        float NdotL = max(dot(N, L), 0.0);
        float NdotH = max(dot(N, H), 0.0);
        
        diffuse += barkColor * NdotL * lights[i].color * lights[i].intensity * attenuation;
        specular += vec3(0.1) * pow(NdotH, 16.0) * lights[i].color * lights[i].intensity * attenuation;
    }
    
    vec3 finalColor = ambient + diffuse + specular;
    
    // gamma correction
    finalColor = pow(finalColor, vec3(1.0/2.2));
    
    FragColor = vec4(finalColor, 1.0);
}
