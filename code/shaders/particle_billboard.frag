#version 330 core

in vec2 TexCoord;
in vec4 ParticleColor;

out vec4 FragColor;

void main() {
    // 計算到中心的距離 (0 = 中心, 1 = 邊緣)
    vec2 center = vec2(0.5, 0.5);
    float dist = length(TexCoord - center) * 2.0;
    
    // 超出範圍直接丟棄
    if (dist > 1.0) {
        discard;
    }
    
    // === Gaussian Fall-off (高斯衰減) ===
    // 使用多層高斯函數創造發光效果
    float sigma1 = 0.1;  // 核心 - 很集中
    float sigma2 = 0.3;  // 中層光暈
    float sigma3 = 0.5;   // 外層柔和光暈
    
    // 高斯函數: exp(-dist^2 / (2 * sigma^2))
    float core = exp(-dist * dist / (2.0 * sigma1 * sigma1));
    float mid = exp(-dist * dist / (2.0 * sigma2 * sigma2));
    float outer = exp(-dist * dist / (2.0 * sigma3 * sigma3));
    
    // 組合多層效果
    // 核心最亮(白色偏移)，中層是主色，外層是柔和光暈
    float coreWeight = 0.15;
    float midWeight = 0.45;
    float outerWeight = 0.25;
    
    float totalGlow = core * coreWeight + mid * midWeight + outer * outerWeight;
    
    // 核心偏白 (增加亮度感)
    vec3 coreColor = mix(ParticleColor.rgb, vec3(1.0), 0.6);  // 核心更白
    vec3 glowColor = ParticleColor.rgb;
    
    // 根據距離混合核心色和光暈色
    vec3 finalColor = mix(glowColor, coreColor, core);
    
    // 最終 alpha 結合粒子透明度和高斯衰減
    float alpha = ParticleColor.a * totalGlow;
    
    // 增加整體亮度 (讓煙火更明亮)
    // finalColor *= 1.5;
    
    FragColor = vec4(finalColor, alpha);
}
