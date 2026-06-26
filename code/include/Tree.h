#ifndef TREE_H
#define TREE_H

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

// 樹枝結構
struct TreeBranch {
    glm::vec3 start;
    glm::vec3 end;
    float radius;
    int level;
    std::vector<glm::vec3> curvePoints;  // 曲線控制點
};

// 葉子粒子結構
struct LeafParticle {
    glm::vec3 position;
    glm::vec3 color;
    float size;
    float rotation;
};

class Tree {
public:
    // 初始化幾何與 shader
    void initialize();
    
    // 設置葉子顏色主題（0=紅色, 1=橘色）
    void setLeafColorTheme(int theme);

    // 設定樹的世界座標變換（位置/縮放/旋轉）
    void setTransform(const glm::vec3& pos, float scale = 1.0f, float rotY = 0.0f);

    // 繪製
    void render(const glm::mat4& view, const glm::mat4& projection, const std::vector<Light>& lights, const Light& env_light, const glm::vec3& viewPos);

private:
    GLuint loadShader(const char* vertPath, const char* fragPath);
    
    // 生成樹枝系統
    void generateBranches();
    void generateBranch(const glm::vec3& start, const glm::vec3& direction, float length, float radius, int level, int maxLevel);
    void addHorizontalBranch(const glm::vec3& start, const glm::vec3& direction, float length, float radius, bool addLeaves);
    void generateBranchesForStyle(int style, const std::vector<glm::vec3>& trunkPoints, float trunkRadius);
    
    // 生成葉子粒子
    void generateLeafParticles();
    void addLeafCluster(const glm::vec3& center, float radius, int count);
    void addLeafClusterFlat(const glm::vec3& center, float radius, int count);  // 水平展開的葉子團
    void addLeafLayer(const glm::vec3& center, const glm::vec3& branchDir, float radius);  // 分層葉子
    
    // 生成樹枝幾何
    void generateBranchGeometry(std::vector<float>& vertices, std::vector<unsigned int>& indices);
    void addCylinder(std::vector<float>& vertices, std::vector<unsigned int>& indices, 
                     const glm::vec3& start, const glm::vec3& end, float radiusStart, float radiusEnd, int segments);
    void addTube(std::vector<float>& vertices, std::vector<unsigned int>& indices,
                 const std::vector<glm::vec3>& points, const std::vector<float>& radii, int segments);
    void addSphere(std::vector<float>& vertices, std::vector<unsigned int>& indices,
                   const glm::vec3& center, float radius, int detail);
    
    // 樹枝和葉子數據
    std::vector<TreeBranch> branches;
    std::vector<LeafParticle> leafParticles;

    // 樹幹渲染
    GLuint branchShaderProgram = 0;
    GLuint branchVAO = 0, branchVBO = 0, branchEBO = 0;
    GLsizei branchIndexCount = 0;
    
    // 葉子 Billboard 渲染
    GLuint leafShaderProgram = 0;
    GLuint leafVAO = 0, leafInstanceVBO = 0;
    GLsizei leafInstanceCount = 0;  // 保存葉子實例數量
    
    glm::vec3 position = glm::vec3(0.0f);
    float scale = 1.0f;
    float rotY = 0.0f;
    int leafColorTheme = 0;  // 0=紅色, 1=橘色
};

#endif