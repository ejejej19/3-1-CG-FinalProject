#ifndef TORII_H
#define TORII_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include "Camera.h"
#include "Light.h"

class Torii {
public:
    void initialize();
    void setTransform(const glm::vec3& pos, float scale = 1.0f, float rotY = 0.0f);
    void render(const glm::mat4& view, const glm::mat4& projection, 
                const std::vector<Light>& lights, const Light& env_light, 
                const glm::vec3& viewPos);

private:
    GLuint loadShader(const char* vertPath, const char* fragPath);
    void addBox(std::vector<float>& vertices, std::vector<unsigned int>& indices,
                const glm::vec3& center, const glm::vec3& size, float partType = 0.0f);
    void addCurvedBeam(std::vector<float>& vertices, std::vector<unsigned int>& indices,
                       const glm::vec3& start, const glm::vec3& end, 
                       float width, float height, float curve, float partType = 0.0f);

    GLuint shaderProgram = 0;
    GLuint toriiVAO = 0, toriiVBO = 0, toriiEBO = 0;
    GLsizei indexCount = 0;
    
    glm::vec3 position = glm::vec3(0.0f);
    float scale = 1.0f;
    float rotY = 0.0f;
};

#endif
