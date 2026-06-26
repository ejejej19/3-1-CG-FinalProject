#ifndef LANTERN_H
#define LANTERN_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include "Camera.h"
#include "Light.h"

class Lantern {
public:
    void initialize();
    void setTransform(const glm::vec3& pos, float scale = 1.0f, float rotY = 0.0f);
    void render(const glm::mat4& view, const glm::mat4& projection, 
                const std::vector<Light>& lights, const Light& env_light, 
                const glm::vec3& viewPos);
    
    // 獲取燈籠的光源信息
    Light getLight() const;

private:
    GLuint loadShader(const char* vertPath, const char* fragPath);
    void addBox(std::vector<float>& vertices, std::vector<unsigned int>& indices,
                const glm::vec3& center, const glm::vec3& size, float isLight = 0.0f);

    GLuint shaderProgram = 0;
    GLuint lanternVAO = 0, lanternVBO = 0, lanternEBO = 0;
    GLsizei indexCount = 0;
    
    glm::vec3 position = glm::vec3(0.0f);
    float scale = 1.0f;
    float rotY = 0.0f;
    
    // 燈光顏色和強度
    glm::vec3 lightColor = glm::vec3(1.0f, 0.85f, 0.6f);  // 暖黃色
    float lightIntensity = 80.0f;
};

#endif
