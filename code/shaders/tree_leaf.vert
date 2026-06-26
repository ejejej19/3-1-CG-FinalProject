#version 330 core
layout (location = 0) in vec3 aPos;           // Quad 頂點位置
layout (location = 1) in vec3 aInstancePos;   // 實例位置
layout (location = 2) in vec3 aInstanceColor; // 實例顏色
layout (location = 3) in float aInstanceSize; // 實例大小
layout (location = 4) in float aInstanceRot;  // 實例旋轉

out vec3 LeafColor;
out vec2 TexCoord;
out float LeafRandom;  // 傳遞隨機值給片段著色器

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 viewPos;

void main() {
    // 計算 Billboard 朝向
    vec3 worldPos = vec3(model * vec4(aInstancePos, 1.0));
    vec3 toCamera = normalize(viewPos - worldPos);
    
    // 構建朝向相機的正交基
    vec3 up = vec3(0, 1, 0);
    vec3 right = normalize(cross(up, toCamera));
    up = cross(toCamera, right);
    
    // 使用旋轉值來旋轉 billboard
    float cosR = cos(aInstanceRot);
    float sinR = sin(aInstanceRot);
    vec2 rotatedPos = vec2(
        aPos.x * cosR - aPos.y * sinR,
        aPos.x * sinR + aPos.y * cosR
    );
    
    // 計算 Billboard 頂點位置
    vec3 vertexPos = worldPos + (right * rotatedPos.x + up * rotatedPos.y) * aInstanceSize;
    
    gl_Position = projection * view * vec4(vertexPos, 1.0);
    
    LeafColor = aInstanceColor;
    TexCoord = aPos.xy + 0.5; // 轉換到 0-1 範圍
    
    // 使用旋轉值作為每片葉子的隨機種子
    LeafRandom = aInstanceRot;
}
