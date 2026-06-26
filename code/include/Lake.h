#ifndef LAKE_H
#define LAKE_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <vector>
#include "Camera.h"
#include "Light.h"
#include "stb_image.h"

class Lake {
public:
    void initialize();
    void render(const glm::mat4& view, const glm::mat4& projection, 
                const glm::mat4& reflViewProj,  // 反射相機的 view-projection 矩陣
                std::vector<Light> lights, Light env_light, float time, GLuint reflectionTexs);
    
    float getWaterLevel() const { return waterLevel; }
    glm::vec4 getClipPlane() const { return glm::vec4(0.0f, 1.0f, 0.0f, -waterLevel); }

private:
    GLuint lakeVAO, lakeVBO, lakeEBO;
    GLuint shaderProgram;
    int indexCount;

    // 湖面反射
    GLuint waterTex = 0;
    GLuint waterNormalTex = 0;
    
    float waterLevel = -0.1f;
    float lakeRadius = 80.0f;  // 放大的湖面

    GLuint loadShader(const char* vertexPath, const char* fragmentPath);
};

#endif
