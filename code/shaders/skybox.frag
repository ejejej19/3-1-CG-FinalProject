#version 330 core
in vec3 TexCoords;

// MRT outputs for HDR + Bloom
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

uniform samplerCube skybox;

uniform float skyExposure;

void main() {
    vec3 skyColor = texture(skybox, TexCoords).rgb;
    // 讓天空更暗，煙火更突出
    skyColor *= skyExposure;
    // skyColor *= 0.5;
    FragColor = vec4(skyColor, 1.0);
    // Skybox 不發光
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
