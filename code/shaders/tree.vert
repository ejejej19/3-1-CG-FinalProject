#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;

    // 法線要用 normal matrix（避免縮放造成錯）
    mat3 normalMat = transpose(inverse(mat3(model)));
    Normal = normalize(normalMat * aNormal);

    gl_Position = projection * view * worldPos;
}
