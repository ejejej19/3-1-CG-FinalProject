#ifndef LAKESHORE_H
#define LAKESHORE_H

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

class LakeShore {
public:
    void initialize(const std::string& texturePath);
    void render(const glm::mat4& view, const glm::mat4& projection, 
                std::vector<Light> lights, Light env_light);

private:
    GLuint shoreVAO, shoreVBO, shoreEBO;
    GLuint shoreTexture;
    GLuint shaderProgram;
    int indexCount;

    GLuint loadTexture(const std::string& path);
    GLuint loadShader(const char* vertexPath, const char* fragmentPath);
};

#endif
