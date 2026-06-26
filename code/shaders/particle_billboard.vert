#version 330 core

// Quad 頂點屬性
layout (location = 0) in vec2 aPos;        // 2D quad 頂點位置
layout (location = 1) in vec2 aTexCoord;   // 紋理座標

// 實例屬性
layout (location = 2) in vec3 instancePos;   // 粒子世界位置
layout (location = 3) in vec4 instanceColor; // 粒子顏色 + 透明度
layout (location = 4) in float instanceSize; // 粒子大小
layout (location = 5) in vec3 instanceVel;   // Phase 4: 粒子速度

out vec2 TexCoord;
out vec4 ParticleColor;

uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraRight;  // 相機右向量
uniform vec3 cameraUp;     // 相機上向量

void main() {
    // Phase 4: Velocity-aligned Billboard
    // 當粒子有速度時，沿速度方向拉伸；靜止時使用標準 billboard
    
    float speed = length(instanceVel);
    float speedThreshold = 0.5;  // 低於此速度使用標準 billboard
    
    vec3 rightDir;
    vec3 upDir;
    float stretchFactor;
    
    if (speed > speedThreshold) {
        // 速度對齊模式：粒子沿速度方向拉伸
        vec3 velocityDir = normalize(instanceVel);
        
        // 計算速度在螢幕空間的投影方向
        // upDir 沿速度方向（拉伸方向）
        // rightDir 垂直於速度且在視圖平面上
        
        // 使用相機的 right 向量來計算垂直於速度的方向
        vec3 sideDir = normalize(cross(velocityDir, cameraRight));
        if (length(cross(velocityDir, cameraRight)) < 0.01) {
            // 如果速度接近平行於 cameraRight，使用 cameraUp
            sideDir = normalize(cross(velocityDir, cameraUp));
        }
        
        // 重新計算確保正交
        rightDir = normalize(cross(velocityDir, sideDir));
        upDir = velocityDir;
        
        // 根據速度拉伸（速度越快，拖尾越長）
        // 使用對數縮放避免過度拉伸
        stretchFactor = 1.0 + log(1.0 + speed * 0.5) * 1.5;
        stretchFactor = clamp(stretchFactor, 1.0, 4.0);  // 限制最大拉伸
    } else {
        // 標準 billboard 模式：面向相機
        rightDir = cameraRight;
        upDir = cameraUp;
        stretchFactor = 1.0;
    }
    
    // 計算頂點位置
    // aPos.x 控制寬度，aPos.y 控制長度（沿速度/上方向）
    vec3 vertexPosition = instancePos 
                        + rightDir * aPos.x * instanceSize 
                        + upDir * aPos.y * instanceSize * stretchFactor;
    
    gl_Position = projection * view * vec4(vertexPosition, 1.0);
    
    TexCoord = aTexCoord;
    ParticleColor = instanceColor;
}
