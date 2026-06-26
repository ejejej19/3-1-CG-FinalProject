#ifndef BILLBOARD_RENDERER_H
#define BILLBOARD_RENDERER_H

#include <vector>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include "Particle.h"
#include "Camera.h"

// 每個粒子的實例數據
struct ParticleInstanceData {
    glm::vec3 position;     // 粒子位置
    glm::vec4 color;        // 顏色 + 透明度
    float size;             // 粒子大小
    glm::vec3 velocity;     // Phase 4: 速度向量，用於對齊和拉伸
};

class BillboardRenderer {
public:
    BillboardRenderer();
    ~BillboardRenderer();

    void initialize();
    void render(const std::vector<Particle>& particles, const glm::mat4& view, const glm::mat4& projection);

private:
    GLuint quadVAO, quadVBO;        // Quad 頂點數據
    GLuint instanceVBO;              // 實例數據 buffer
    GLuint shaderProgram;
    
    static const int MAX_PARTICLES = 100000;  // 最大粒子數
    
    GLuint compileShader(const char* source, GLenum shaderType);
    GLuint loadShaders(const char* vertexPath, const char* fragmentPath);
};

#endif
