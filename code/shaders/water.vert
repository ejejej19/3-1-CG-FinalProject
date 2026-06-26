#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in vec3 aNormal;

out vec2 TexCoords;
out vec3 FragPos;
out vec3 Normal;
out vec4 ClipSpacePos;
out vec4 ReflClipPos;  // 反射相機投影後的位置

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 reflViewProj;  // 反射相機的 view-projection 矩陣
uniform float time;
uniform float lakeRadius;

void main() {
    // 計算距離中心的距離，用於邊緣衰減
    float distFromCenter = length(aPos.xz);
    float edgeFade = 1.0 - smoothstep(lakeRadius * 0.7, lakeRadius * 0.95, distFromCenter);
    
    // 加入輕微的波浪效果，邊緣處衰減
    vec3 pos = aPos;
    float wave1 = sin(pos.x * 0.5 + time * 2.0) * 0.02;
    float wave2 = sin(pos.z * 0.3 + time * 1.5) * 0.02;
    float wave3 = sin((pos.x + pos.z) * 0.4 + time * 2.5) * 0.015;
    pos.y += (wave1 + wave2 + wave3) * edgeFade;
    
    // 計算波浪後的法線，同樣邊緣衰減
    float dx = (cos(pos.x * 0.5 + time * 2.0) * 0.5 * 0.02 
             + cos((pos.x + pos.z) * 0.4 + time * 2.5) * 0.4 * 0.015) * edgeFade;
    float dz = (cos(pos.z * 0.3 + time * 1.5) * 0.3 * 0.02 
             + cos((pos.x + pos.z) * 0.4 + time * 2.5) * 0.4 * 0.015) * edgeFade;
    vec3 waveNormal = normalize(vec3(-dx, 1.0, -dz));
    
    vec4 worldPos = model * vec4(pos, 1.0);
    FragPos = vec3(worldPos);
    Normal = mat3(transpose(inverse(model))) * waveNormal;
    TexCoords = aTexCoords;
    ClipSpacePos = projection * view * worldPos;
    
    // 計算反射相機投影座標 - 用於精確的反射貼圖採樣
    ReflClipPos = reflViewProj * worldPos;
    
    gl_Position = ClipSpacePos;
}
