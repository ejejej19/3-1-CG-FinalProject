#ifndef HOUSE_H
#define HOUSE_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

#include "Light.h"

class House {
public:
    House();
    ~House();
    
    void initialize();
    void render(const glm::mat4& view, const glm::mat4& projection,
                std::vector<Light> lights, Light env_light,
                const glm::vec3& viewPos, float time);
    
    void setPosition(const glm::vec3& pos) { position = pos; }
    void setRotation(float angle) { rotationY = angle; }
    void setScale(float s) { scale = s; }
    
    glm::vec3 getPosition() const { return position; }
    
private:
    GLuint houseVAO, houseVBO, houseEBO;
    GLuint shaderProgram;
    unsigned int indexCount;
    
    glm::vec3 position;
    float rotationY;
    float scale;
    
    // 材質類型: 0=紅色柱子, 1=發光燈籠(暖黃), 2=屋頂(深灰), 3=地板(淺棕)
    void addBox(std::vector<float>& vertices, std::vector<unsigned int>& indices,
                const glm::vec3& center, const glm::vec3& size, float matType);
    void addRoof(std::vector<float>& vertices, std::vector<unsigned int>& indices,
                 float baseY, float width, float depth, float height, int layers);
    void addWindowArch(std::vector<float>& vertices, std::vector<unsigned int>& indices,
                       const glm::vec3& center, float width, float height, float depth, 
                       float matType, bool horizontal);
};

#endif
