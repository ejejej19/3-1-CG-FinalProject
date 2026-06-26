#ifndef MOUNTAIN_H
#define MOUNTAIN_H

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

class Mountain {
public:
    void initialize();
    void render(const glm::mat4& view, const glm::mat4& projection, 
                std::vector<Light> lights, Light env_light);

private:
    GLuint mountainVAO, mountainVBO, mountainEBO;
    GLuint shaderProgram;
    int indexCount;
    
    // 紋理
    GLuint rockTexture;
    GLuint snowTexture;

    GLuint loadShader(const char* vertexPath, const char* fragmentPath);
    GLuint loadTexture(const char* path);
    float getHeight(float x, float z);
};

#endif
