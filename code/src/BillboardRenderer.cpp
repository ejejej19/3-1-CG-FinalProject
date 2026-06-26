#include "BillboardRenderer.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

BillboardRenderer::BillboardRenderer() 
    : quadVAO(0), quadVBO(0), instanceVBO(0), shaderProgram(0) {}

BillboardRenderer::~BillboardRenderer() {
    if (quadVAO) glDeleteVertexArrays(1, &quadVAO);
    if (quadVBO) glDeleteBuffers(1, &quadVBO);
    if (instanceVBO) glDeleteBuffers(1, &instanceVBO);
    if (shaderProgram) glDeleteProgram(shaderProgram);
}

GLuint BillboardRenderer::compileShader(const char* source, GLenum shaderType) {
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Billboard Shader Compilation Failed: " << infoLog << std::endl;
    }
    return shader;
}

GLuint BillboardRenderer::loadShaders(const char* vertexPath, const char* fragmentPath) {
    // 讀取 shader 檔案
    std::string vertexCode, fragmentCode;
    std::ifstream vShaderFile, fShaderFile;
    
    vShaderFile.open(vertexPath);
    fShaderFile.open(fragmentPath);
    
    if (!vShaderFile.is_open() || !fShaderFile.is_open()) {
        std::cerr << "Failed to open billboard shader files" << std::endl;
        return 0;
    }
    
    std::stringstream vShaderStream, fShaderStream;
    vShaderStream << vShaderFile.rdbuf();
    fShaderStream << fShaderFile.rdbuf();
    vShaderFile.close();
    fShaderFile.close();
    
    vertexCode = vShaderStream.str();
    fragmentCode = fShaderStream.str();

    GLuint vertexShader = compileShader(vertexCode.c_str(), GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader(fragmentCode.c_str(), GL_FRAGMENT_SHADER);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Billboard Shader Linking Failed: " << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

void BillboardRenderer::initialize() {
    // Billboard quad 頂點數據 (2D quad，中心在原點)
    // 每個頂點: position(2) + texCoord(2)
    float quadVertices[] = {
        // positions   // texCoords
        -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  1.0f, 1.0f,
        
        -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.0f, 1.0f
    };

    // 創建 VAO 和 VBO
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glGenBuffers(1, &instanceVBO);

    glBindVertexArray(quadVAO);

    // Quad 頂點數據
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // 頂點位置 (location = 0)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // 紋理座標 (location = 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // 實例數據 buffer
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * sizeof(ParticleInstanceData), nullptr, GL_DYNAMIC_DRAW);

    // 實例位置 (location = 2)
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleInstanceData), (void*)0);
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);  // 每個實例更新一次

    // 實例顏色 (location = 3)
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleInstanceData), (void*)offsetof(ParticleInstanceData, color));
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    // 實例大小 (location = 4)
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleInstanceData), (void*)offsetof(ParticleInstanceData, size));
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);

    // Phase 4: 實例速度 (location = 5)
    glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleInstanceData), (void*)offsetof(ParticleInstanceData, velocity));
    glEnableVertexAttribArray(5);
    glVertexAttribDivisor(5, 1);

    glBindVertexArray(0);

    // 載入 shaders
    shaderProgram = loadShaders("../shaders/particle_billboard.vert", "../shaders/particle_billboard.frag");
}

void BillboardRenderer::render(const std::vector<Particle>& particles, 
                                const glm::mat4& view, 
                                const glm::mat4& projection) {
    if (particles.empty()) return;

    // 準備實例數據
    std::vector<ParticleInstanceData> instanceData;
    instanceData.reserve(particles.size());

    for (const auto& particle : particles) {
        ParticleInstanceData data;
        data.position = particle.getPosition();
        data.color = glm::vec4(particle.getColor(), particle.getTransparency());
        data.size = 1.0f;  // 放大粒子大小
        data.velocity = particle.getVelocity();  // Phase 4: 速度對齊
        instanceData.push_back(data);
    }

    // 按深度排序 (從遠到近)，確保正確的透明度混合
    glm::vec3 cameraPos = Camera::position;
    std::sort(instanceData.begin(), instanceData.end(),
        [&cameraPos](const ParticleInstanceData& a, const ParticleInstanceData& b) {
            float distA = glm::length(a.position - cameraPos);
            float distB = glm::length(b.position - cameraPos);
            return distA > distB;  // 遠的先渲染
        });

    // 上傳實例數據
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, instanceData.size() * sizeof(ParticleInstanceData), instanceData.data());

    // 設定渲染狀態
    glEnable(GL_BLEND);
    // Additive Blending: 重疊的粒子會疊加變亮，產生發光效果
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);  // 禁用深度寫入，但保留深度測試

    // 使用 shader
    glUseProgram(shaderProgram);

    // 傳遞 uniform
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    // 傳遞相機的 right 和 up 向量用於 billboard 計算
    glm::vec3 cameraRight = glm::vec3(view[0][0], view[1][0], view[2][0]);
    glm::vec3 cameraUp = glm::vec3(view[0][1], view[1][1], view[2][1]);
    glUniform3fv(glGetUniformLocation(shaderProgram, "cameraRight"), 1, glm::value_ptr(cameraRight));
    glUniform3fv(glGetUniformLocation(shaderProgram, "cameraUp"), 1, glm::value_ptr(cameraUp));

    // 繪製所有粒子 (instanced)
    glBindVertexArray(quadVAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, instanceData.size());
    glBindVertexArray(0);

    // 恢復渲染狀態
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}
