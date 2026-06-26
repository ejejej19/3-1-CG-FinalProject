#include "House.h"
#include "utils.h"
#include <iostream>
#include <cmath>

House::House() : houseVAO(0), houseVBO(0), houseEBO(0), shaderProgram(0),
                 indexCount(0), position(0.0f), rotationY(0.0f), scale(1.0f) {
}

House::~House() {
    if (houseVAO) glDeleteVertexArrays(1, &houseVAO);
    if (houseVBO) glDeleteBuffers(1, &houseVBO);
    if (houseEBO) glDeleteBuffers(1, &houseEBO);
    if (shaderProgram) glDeleteProgram(shaderProgram);
}

void House::addBox(std::vector<float>& vertices, std::vector<unsigned int>& indices,
                   const glm::vec3& center, const glm::vec3& size, float matType) {
    float hx = size.x * 0.5f;
    float hy = size.y * 0.5f;
    float hz = size.z * 0.5f;
    
    glm::vec3 corners[8] = {
        center + glm::vec3(-hx, -hy, -hz),
        center + glm::vec3( hx, -hy, -hz),
        center + glm::vec3( hx,  hy, -hz),
        center + glm::vec3(-hx,  hy, -hz),
        center + glm::vec3(-hx, -hy,  hz),
        center + glm::vec3( hx, -hy,  hz),
        center + glm::vec3( hx,  hy,  hz),
        center + glm::vec3(-hx,  hy,  hz)
    };
    
    glm::vec3 normals[6] = {
        glm::vec3( 0,  0, -1),
        glm::vec3( 0,  0,  1),
        glm::vec3(-1,  0,  0),
        glm::vec3( 1,  0,  0),
        glm::vec3( 0,  1,  0),
        glm::vec3( 0, -1,  0)
    };
    
    int faceIndices[6][4] = {
        {0, 1, 2, 3},
        {5, 4, 7, 6},
        {4, 0, 3, 7},
        {1, 5, 6, 2},
        {3, 2, 6, 7},
        {4, 5, 1, 0}
    };
    
    unsigned int baseIndex = vertices.size() / 7;
    
    for (int face = 0; face < 6; face++) {
        for (int v = 0; v < 4; v++) {
            glm::vec3 pos = corners[faceIndices[face][v]];
            glm::vec3 norm = normals[face];
            vertices.push_back(pos.x);
            vertices.push_back(pos.y);
            vertices.push_back(pos.z);
            vertices.push_back(norm.x);
            vertices.push_back(norm.y);
            vertices.push_back(norm.z);
            vertices.push_back(matType);
        }
        
        unsigned int faceBase = baseIndex + face * 4;
        indices.push_back(faceBase + 0);
        indices.push_back(faceBase + 1);
        indices.push_back(faceBase + 2);
        indices.push_back(faceBase + 0);
        indices.push_back(faceBase + 2);
        indices.push_back(faceBase + 3);
    }
}

// 翹角屋頂 - 四角微微上翹，更多層數更平滑
void House::addRoof(std::vector<float>& vertices, std::vector<unsigned int>& indices,
                    float baseY, float width, float depth, float height, int layers) {
    // 簡潔的分層屋頂，沒有翹角效果
    int actualLayers = layers;
    float layerHeight = height / actualLayers;
    
    for (int i = 0; i < actualLayers; i++) {
        float t = (float)i / (actualLayers - 1);
        // 使用平滑曲線讓收縮更自然
        float smoothT = t * t * (3.0f - 2.0f * t);  // smoothstep
        float layerWidth = width * (1.0f - smoothT * 0.7f);
        float layerDepth = depth * (1.0f - smoothT * 0.75f);
        float layerY = baseY + i * layerHeight;
        
        // 主體屋頂層
        addBox(vertices, indices, glm::vec3(0, layerY, 0),
               glm::vec3(layerWidth, layerHeight * 1.01f, layerDepth), 2.0f);
    }
}

// 添加實心半圓形窗戶頂部
void House::addWindowArch(std::vector<float>& vertices, std::vector<unsigned int>& indices,
                          const glm::vec3& center, float width, float height, float depth,
                          float matType, bool horizontal) {
    const int segments = 16;  // 更多分段讓半圓更平滑
    
    if (horizontal) {
        // 水平方向的實心半圓（用於Z方向的窗戶）
        float hw = depth * 0.5f;  // 半深度
        
        // 前面（X = -hw）
        {
            glm::vec3 normal(-1, 0, 0);
            glm::vec3 centerFront = center + glm::vec3(-hw, 0, 0);
            unsigned int centerIdx = vertices.size() / 7;
            
            // 添加中心點
            vertices.push_back(centerFront.x);
            vertices.push_back(centerFront.y);
            vertices.push_back(centerFront.z);
            vertices.push_back(normal.x);
            vertices.push_back(normal.y);
            vertices.push_back(normal.z);
            vertices.push_back(matType);
            
            // 添加半圓弧上的點
            for (int i = 0; i <= segments; i++) {
                float t = (float)i / segments;
                float angle = 3.14159f * t;  // 0 到 π
                float z = (width * 0.5f) * std::cos(angle);
                float y = height * std::sin(angle);
                
                vertices.push_back(centerFront.x);
                vertices.push_back(centerFront.y + y);
                vertices.push_back(centerFront.z + z);
                vertices.push_back(normal.x);
                vertices.push_back(normal.y);
                vertices.push_back(normal.z);
                vertices.push_back(matType);
                
                // 創建三角形扇形
                if (i > 0) {
                    indices.push_back(centerIdx);
                    indices.push_back(centerIdx + i);
                    indices.push_back(centerIdx + i + 1);
                }
            }
        }
        
        // 後面（X = hw）
        {
            glm::vec3 normal(1, 0, 0);
            glm::vec3 centerBack = center + glm::vec3(hw, 0, 0);
            unsigned int centerIdx = vertices.size() / 7;
            
            // 添加中心點
            vertices.push_back(centerBack.x);
            vertices.push_back(centerBack.y);
            vertices.push_back(centerBack.z);
            vertices.push_back(normal.x);
            vertices.push_back(normal.y);
            vertices.push_back(normal.z);
            vertices.push_back(matType);
            
            // 添加半圓弧上的點
            for (int i = 0; i <= segments; i++) {
                float t = (float)i / segments;
                float angle = 3.14159f * t;
                float z = (width * 0.5f) * std::cos(angle);
                float y = height * std::sin(angle);
                
                vertices.push_back(centerBack.x);
                vertices.push_back(centerBack.y + y);
                vertices.push_back(centerBack.z + z);
                vertices.push_back(normal.x);
                vertices.push_back(normal.y);
                vertices.push_back(normal.z);
                vertices.push_back(matType);
                
                // 創建三角形扇形（反向繞序）
                if (i > 0) {
                    indices.push_back(centerIdx);
                    indices.push_back(centerIdx + i + 1);
                    indices.push_back(centerIdx + i);
                }
            }
        }
        
        // 側面 - 連接前後兩個半圓的邊緣
        for (int i = 0; i <= segments; i++) {
            float t = (float)i / segments;
            float angle = 3.14159f * t;
            float z = (width * 0.5f) * std::cos(angle);
            float y = height * std::sin(angle);
            
            glm::vec3 posFront = center + glm::vec3(-hw, y, z);
            glm::vec3 posBack = center + glm::vec3(hw, y, z);
            
            // 計算法向量（指向圓弧外側）
            glm::vec3 normal(0, std::sin(angle), std::cos(angle));
            
            unsigned int baseIdx = vertices.size() / 7;
            
            // 前面的點
            vertices.push_back(posFront.x);
            vertices.push_back(posFront.y);
            vertices.push_back(posFront.z);
            vertices.push_back(normal.x);
            vertices.push_back(normal.y);
            vertices.push_back(normal.z);
            vertices.push_back(matType);
            
            // 後面的點
            vertices.push_back(posBack.x);
            vertices.push_back(posBack.y);
            vertices.push_back(posBack.z);
            vertices.push_back(normal.x);
            vertices.push_back(normal.y);
            vertices.push_back(normal.z);
            vertices.push_back(matType);
            
            // 連接相鄰分段
            if (i > 0) {
                unsigned int prevBase = baseIdx - 2;
                indices.push_back(prevBase);
                indices.push_back(baseIdx);
                indices.push_back(baseIdx + 1);
                indices.push_back(prevBase);
                indices.push_back(baseIdx + 1);
                indices.push_back(prevBase + 1);
            }
        }
        
    } else {
        // 垂直方向的實心半圓（用於X方向的窗戶）
        float hw = depth * 0.5f;  // 半深度
        
        // 前面（Z = -hw）
        {
            glm::vec3 normal(0, 0, -1);
            glm::vec3 centerFront = center + glm::vec3(0, 0, -hw);
            unsigned int centerIdx = vertices.size() / 7;
            
            // 添加中心點
            vertices.push_back(centerFront.x);
            vertices.push_back(centerFront.y);
            vertices.push_back(centerFront.z);
            vertices.push_back(normal.x);
            vertices.push_back(normal.y);
            vertices.push_back(normal.z);
            vertices.push_back(matType);
            
            // 添加半圓弧上的點
            for (int i = 0; i <= segments; i++) {
                float t = (float)i / segments;
                float angle = 3.14159f * t;
                float x = (width * 0.5f) * std::cos(angle);
                float y = height * std::sin(angle);
                
                vertices.push_back(centerFront.x + x);
                vertices.push_back(centerFront.y + y);
                vertices.push_back(centerFront.z);
                vertices.push_back(normal.x);
                vertices.push_back(normal.y);
                vertices.push_back(normal.z);
                vertices.push_back(matType);
                
                // 創建三角形扇形
                if (i > 0) {
                    indices.push_back(centerIdx);
                    indices.push_back(centerIdx + i);
                    indices.push_back(centerIdx + i + 1);
                }
            }
        }
        
        // 後面（Z = hw）
        {
            glm::vec3 normal(0, 0, 1);
            glm::vec3 centerBack = center + glm::vec3(0, 0, hw);
            unsigned int centerIdx = vertices.size() / 7;
            
            // 添加中心點
            vertices.push_back(centerBack.x);
            vertices.push_back(centerBack.y);
            vertices.push_back(centerBack.z);
            vertices.push_back(normal.x);
            vertices.push_back(normal.y);
            vertices.push_back(normal.z);
            vertices.push_back(matType);
            
            // 添加半圓弧上的點
            for (int i = 0; i <= segments; i++) {
                float t = (float)i / segments;
                float angle = 3.14159f * t;
                float x = (width * 0.5f) * std::cos(angle);
                float y = height * std::sin(angle);
                
                vertices.push_back(centerBack.x + x);
                vertices.push_back(centerBack.y + y);
                vertices.push_back(centerBack.z);
                vertices.push_back(normal.x);
                vertices.push_back(normal.y);
                vertices.push_back(normal.z);
                vertices.push_back(matType);
                
                // 創建三角形扇形（反向繞序）
                if (i > 0) {
                    indices.push_back(centerIdx);
                    indices.push_back(centerIdx + i + 1);
                    indices.push_back(centerIdx + i);
                }
            }
        }
        
        // 側面 - 連接前後兩個半圓的邊緣
        for (int i = 0; i <= segments; i++) {
            float t = (float)i / segments;
            float angle = 3.14159f * t;
            float x = (width * 0.5f) * std::cos(angle);
            float y = height * std::sin(angle);
            
            glm::vec3 posFront = center + glm::vec3(x, y, -hw);
            glm::vec3 posBack = center + glm::vec3(x, y, hw);
            
            // 計算法向量（指向圓弧外側）
            glm::vec3 normal(std::cos(angle), std::sin(angle), 0);
            
            unsigned int baseIdx = vertices.size() / 7;
            
            // 前面的點
            vertices.push_back(posFront.x);
            vertices.push_back(posFront.y);
            vertices.push_back(posFront.z);
            vertices.push_back(normal.x);
            vertices.push_back(normal.y);
            vertices.push_back(normal.z);
            vertices.push_back(matType);
            
            // 後面的點
            vertices.push_back(posBack.x);
            vertices.push_back(posBack.y);
            vertices.push_back(posBack.z);
            vertices.push_back(normal.x);
            vertices.push_back(normal.y);
            vertices.push_back(normal.z);
            vertices.push_back(matType);
            
            // 連接相鄰分段
            if (i > 0) {
                unsigned int prevBase = baseIdx - 2;
                indices.push_back(prevBase);
                indices.push_back(baseIdx);
                indices.push_back(baseIdx + 1);
                indices.push_back(prevBase);
                indices.push_back(baseIdx + 1);
                indices.push_back(prevBase + 1);
            }
        }
    }
}

void House::initialize() {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    // ======== 金閣寺風格參數 ========
    // 材質: 0=深棕木柱子, 1=金色牆面, 2=深色屋頂, 3=淺棕地板
    // 正面朝+Z方向（面對湖）
    
    float groundY = 0.0f;
    
    // 第一層（書院造）- 深棕色木質
    float floor1Width = 22.0f;
    float floor1Depth = 14.0f;
    float floor1Height = 3.5f;
    float floor1Y = groundY + 0.6f;
    
    // 第二層（武家造）- 金色
    float floor2Width = 18.0f;
    float floor2Depth = 11.0f;
    float floor2Height = 4.2f;
    
    // 第三層（禪宗佛殿造）- 金色
    float floor3Width = 10.0f;
    float floor3Depth = 7.0f;
    float floor3Height = 3.6f;
    
    float pillarRadius = 0.25f;
    float wallThick = 0.15f;
    
    // ======== 0. 基座石台 ========
    addBox(vertices, indices, glm::vec3(0, groundY + 0.15f, 0),
           glm::vec3(floor1Width + 4.0f, 0.3f, floor1Depth + 4.0f), 2.0f);
    addBox(vertices, indices, glm::vec3(0, groundY + 0.4f, 0),
           glm::vec3(floor1Width + 2.0f, 0.2f, floor1Depth + 2.0f), 2.0f);
    
    // ======== 第一層（書院造 - 深棕色）========
    float base1 = floor1Y;
    
    // 地板
    addBox(vertices, indices, glm::vec3(0, base1, 0),
           glm::vec3(floor1Width, 0.15f, floor1Depth), 3.0f);
    
    // 柱子位置
    float col1X = floor1Width * 0.47f;
    float col1Z = floor1Depth * 0.47f;
    float colY1 = base1 + floor1Height * 0.5f;
    
    // 四角柱
    addBox(vertices, indices, glm::vec3(-col1X, colY1, -col1Z),
           glm::vec3(pillarRadius * 1.3f, floor1Height, pillarRadius * 1.3f), 0.0f);
    addBox(vertices, indices, glm::vec3(col1X, colY1, -col1Z),
           glm::vec3(pillarRadius * 1.3f, floor1Height, pillarRadius * 1.3f), 0.0f);
    addBox(vertices, indices, glm::vec3(-col1X, colY1, col1Z),
           glm::vec3(pillarRadius * 1.3f, floor1Height, pillarRadius * 1.3f), 0.0f);
    addBox(vertices, indices, glm::vec3(col1X, colY1, col1Z),
           glm::vec3(pillarRadius * 1.3f, floor1Height, pillarRadius * 1.3f), 0.0f);
    
    // 中間柱子
    for (float x = -floor1Width * 0.25f; x <= floor1Width * 0.25f; x += floor1Width * 0.25f) {
        addBox(vertices, indices, glm::vec3(x, colY1, -col1Z),
               glm::vec3(pillarRadius, floor1Height, pillarRadius), 0.0f);
        addBox(vertices, indices, glm::vec3(x, colY1, col1Z),
               glm::vec3(pillarRadius, floor1Height, pillarRadius), 0.0f);
    }
    for (float z = -floor1Depth * 0.25f; z <= floor1Depth * 0.25f; z += floor1Depth * 0.5f) {
        addBox(vertices, indices, glm::vec3(-col1X, colY1, z),
               glm::vec3(pillarRadius, floor1Height, pillarRadius), 0.0f);
        addBox(vertices, indices, glm::vec3(col1X, colY1, z),
               glm::vec3(pillarRadius, floor1Height, pillarRadius), 0.0f);
    }
    
    // 第一層牆面（深棕色木質牆）
    float wall1Y = base1 + floor1Height * 0.5f;
    // 後牆
    addBox(vertices, indices, glm::vec3(0, wall1Y, -floor1Depth * 0.47f),
           glm::vec3(floor1Width * 0.93f, floor1Height, wallThick), 0.0f);
    // 前牆
    addBox(vertices, indices, glm::vec3(0, wall1Y, floor1Depth * 0.47f),
           glm::vec3(floor1Width * 0.93f, floor1Height, wallThick), 0.0f);
    // 左右牆
    addBox(vertices, indices, glm::vec3(-floor1Width * 0.47f, wall1Y, 0),
           glm::vec3(wallThick, floor1Height, floor1Depth * 0.93f), 0.0f);
    addBox(vertices, indices, glm::vec3(floor1Width * 0.47f, wall1Y, 0),
           glm::vec3(wallThick, floor1Height, floor1Depth * 0.93f), 0.0f);
    
    // 第一層門與窗戶裝飾
    // 正面大門（中央，面向湖）
    float doorY1 = base1 + floor1Height * 0.45f;
    addBox(vertices, indices, glm::vec3(0, doorY1, floor1Depth * 0.48f),
           glm::vec3(2.5f, 2.8f, 0.1f), 3.0f);
    
    // 前面側窗（大門兩側）
    for (float x = -floor1Width * 0.32f; x <= floor1Width * 0.32f; x += floor1Width * 0.64f) {
        float winY = base1 + floor1Height * 0.6f;
        addBox(vertices, indices, glm::vec3(x, winY, floor1Depth * 0.48f),
               glm::vec3(1.8f, 2.0f, 0.1f), 3.0f);
    }
    
    // 後牆窗戶（對稱分布）
    for (float x = -floor1Width * 0.3f; x <= floor1Width * 0.3f; x += floor1Width * 0.3f) {
        float winY = base1 + floor1Height * 0.6f;
        addBox(vertices, indices, glm::vec3(x, winY, -floor1Depth * 0.48f),
               glm::vec3(1.6f, 1.8f, 0.1f), 3.0f);
    }
    
    // 左右牆窗戶
    for (float z = -floor1Depth * 0.25f; z <= floor1Depth * 0.25f; z += floor1Depth * 0.5f) {
        float winY = base1 + floor1Height * 0.6f;
        addBox(vertices, indices, glm::vec3(-floor1Width * 0.48f, winY, z),
               glm::vec3(0.1f, 1.6f, 1.4f), 3.0f);
        addBox(vertices, indices, glm::vec3(floor1Width * 0.48f, winY, z),
               glm::vec3(0.1f, 1.6f, 1.4f), 3.0f);
    }
    
    // 欄杆（深棕色）- 簡潔的日式欄杆
    float railY1 = base1 + 0.55f;
    float railThick = 0.06f;
    float railHeight = 0.7f;
    
    // 上下橫杆（前後）
    addBox(vertices, indices, glm::vec3(0, railY1, col1Z),
           glm::vec3(floor1Width * 0.94f, railThick, railThick), 4.0f);
    addBox(vertices, indices, glm::vec3(0, railY1, -col1Z),
           glm::vec3(floor1Width * 0.94f, railThick, railThick), 4.0f);
    addBox(vertices, indices, glm::vec3(0, railY1 - railHeight + 0.1f, col1Z),
           glm::vec3(floor1Width * 0.94f, railThick, railThick), 4.0f);
    addBox(vertices, indices, glm::vec3(0, railY1 - railHeight + 0.1f, -col1Z),
           glm::vec3(floor1Width * 0.94f, railThick, railThick), 4.0f);
    
    // 上下橫杆（左右）
    addBox(vertices, indices, glm::vec3(-col1X, railY1, 0),
           glm::vec3(railThick, railThick, floor1Depth * 0.94f), 4.0f);
    addBox(vertices, indices, glm::vec3(col1X, railY1, 0),
           glm::vec3(railThick, railThick, floor1Depth * 0.94f), 4.0f);
    addBox(vertices, indices, glm::vec3(-col1X, railY1 - railHeight + 0.1f, 0),
           glm::vec3(railThick, railThick, floor1Depth * 0.94f), 4.0f);
    addBox(vertices, indices, glm::vec3(col1X, railY1 - railHeight + 0.1f, 0),
           glm::vec3(railThick, railThick, floor1Depth * 0.94f), 4.0f);
    
    // 欄杆垂直柱（間隔更大，更美觀，避免與主柱重疊）
    float postSpacing1 = 2.0f;
    for (float x = -floor1Width * 0.44f; x <= floor1Width * 0.44f; x += postSpacing1) {
        // 避免在主柱位置放置欄杆柱
        if (std::abs(x - col1X) > 0.5f && std::abs(x + col1X) > 0.5f) {
            addBox(vertices, indices, glm::vec3(x, railY1 - railHeight * 0.5f + 0.05f, col1Z),
                   glm::vec3(railThick * 0.8f, railHeight, railThick * 0.8f), 4.0f);
            addBox(vertices, indices, glm::vec3(x, railY1 - railHeight * 0.5f + 0.05f, -col1Z),
                   glm::vec3(railThick * 0.8f, railHeight, railThick * 0.8f), 4.0f);
        }
    }
    for (float z = -floor1Depth * 0.35f; z <= floor1Depth * 0.35f; z += postSpacing1) {
        // 避免在主柱位置放置欄杆柱
        if (std::abs(z - col1Z) > 0.5f && std::abs(z + col1Z) > 0.5f) {
            addBox(vertices, indices, glm::vec3(-col1X, railY1 - railHeight * 0.5f + 0.05f, z),
                   glm::vec3(railThick * 0.8f, railHeight, railThick * 0.8f), 4.0f);
            addBox(vertices, indices, glm::vec3(col1X, railY1 - railHeight * 0.5f + 0.05f, z),
                   glm::vec3(railThick * 0.8f, railHeight, railThick * 0.8f), 4.0f);
        }
    }
    
    // 頂部橫樑
    float beam1Y = base1 + floor1Height;
    addBox(vertices, indices, glm::vec3(0, beam1Y, -col1Z),
           glm::vec3(floor1Width + 0.5f, 0.25f, 0.3f), 0.0f);
    addBox(vertices, indices, glm::vec3(0, beam1Y, col1Z),
           glm::vec3(floor1Width + 0.5f, 0.25f, 0.3f), 0.0f);
    addBox(vertices, indices, glm::vec3(-col1X, beam1Y, 0),
           glm::vec3(0.3f, 0.25f, floor1Depth + 0.5f), 0.0f);
    addBox(vertices, indices, glm::vec3(col1X, beam1Y, 0),
           glm::vec3(0.3f, 0.25f, floor1Depth + 0.5f), 0.0f);
    
    // 第一層屋頂
    float roof1Base = beam1Y + 0.15f;
    float roof1Height = 1.8f;
    float roof1Overhang = 2.5f;
    addRoof(vertices, indices, roof1Base, floor1Width + roof1Overhang * 2, 
            floor1Depth + roof1Overhang * 2, roof1Height, 20);
    
    // ======== 第二層（武家造 - 金色）========
    float base2 = roof1Base + roof1Height * 0.15f;
    
    // 金色牆面（四面封閉）
    float wall2Y = base2 + floor2Height * 0.5f;
    // 後牆
    addBox(vertices, indices, glm::vec3(0, wall2Y, -floor2Depth * 0.45f),
           glm::vec3(floor2Width * 0.95f, floor2Height, wallThick), 1.0f);
    // 前牆（現在有了！）
    addBox(vertices, indices, glm::vec3(0, wall2Y, floor2Depth * 0.45f),
           glm::vec3(floor2Width * 0.95f, floor2Height, wallThick), 1.0f);
    // 左右牆
    addBox(vertices, indices, glm::vec3(-floor2Width * 0.47f, wall2Y, 0),
           glm::vec3(wallThick, floor2Height, floor2Depth * 0.9f), 1.0f);
    addBox(vertices, indices, glm::vec3(floor2Width * 0.47f, wall2Y, 0),
           glm::vec3(wallThick, floor2Height, floor2Depth * 0.9f), 1.0f);
    
    // 地板
    addBox(vertices, indices, glm::vec3(0, base2, 0),
           glm::vec3(floor2Width, 0.12f, floor2Depth), 3.0f);
    
    // 金色角柱
    float col2X = floor2Width * 0.47f;
    float col2Z = floor2Depth * 0.47f;
    float colY2 = base2 + floor2Height * 0.5f;
    
    addBox(vertices, indices, glm::vec3(-col2X, colY2, -col2Z),
           glm::vec3(pillarRadius * 1.2f, floor2Height, pillarRadius * 1.2f), 1.0f);
    addBox(vertices, indices, glm::vec3(col2X, colY2, -col2Z),
           glm::vec3(pillarRadius * 1.2f, floor2Height, pillarRadius * 1.2f), 1.0f);
    addBox(vertices, indices, glm::vec3(-col2X, colY2, col2Z),
           glm::vec3(pillarRadius * 1.2f, floor2Height, pillarRadius * 1.2f), 1.0f);
    addBox(vertices, indices, glm::vec3(col2X, colY2, col2Z),
           glm::vec3(pillarRadius * 1.2f, floor2Height, pillarRadius * 1.2f), 1.0f);
    
    // ====== 第二層外推陽台欄杆 ======
    float balcony2Offset = 1.5f;
    float balcony2Width = floor2Width + balcony2Offset * 2;
    float balcony2Depth = floor2Depth + balcony2Offset * 2;
    float balcony2Floor = base2 - 0.1f;
    
    // 陽台地板
    addBox(vertices, indices, glm::vec3(0, balcony2Floor, 0),
           glm::vec3(balcony2Width, 0.1f, balcony2Depth), 3.0f);
    
    // 陽台欄杆（簡潔日式風格）
    float railY2 = balcony2Floor + 1.2f;
    float rail2Height = 0.65f;
    float rail2Thick = 0.05f;
    
    // 上下橫杆（四面）
    // 前後
    addBox(vertices, indices, glm::vec3(0, railY2, balcony2Depth * 0.48f),
           glm::vec3(balcony2Width * 0.95f, rail2Thick, rail2Thick), 4.0f);
    addBox(vertices, indices, glm::vec3(0, railY2, -balcony2Depth * 0.48f),
           glm::vec3(balcony2Width * 0.95f, rail2Thick, rail2Thick), 4.0f);
    addBox(vertices, indices, glm::vec3(0, railY2 - rail2Height + 0.08f, balcony2Depth * 0.48f),
           glm::vec3(balcony2Width * 0.95f, rail2Thick, rail2Thick), 4.0f);
    addBox(vertices, indices, glm::vec3(0, railY2 - rail2Height + 0.08f, -balcony2Depth * 0.48f),
           glm::vec3(balcony2Width * 0.95f, rail2Thick, rail2Thick), 4.0f);
    // 左右
    addBox(vertices, indices, glm::vec3(-balcony2Width * 0.48f, railY2, 0),
           glm::vec3(rail2Thick, rail2Thick, balcony2Depth * 0.95f), 4.0f);
    addBox(vertices, indices, glm::vec3(balcony2Width * 0.48f, railY2, 0),
           glm::vec3(rail2Thick, rail2Thick, balcony2Depth * 0.95f), 4.0f);
    addBox(vertices, indices, glm::vec3(-balcony2Width * 0.48f, railY2 - rail2Height + 0.08f, 0),
           glm::vec3(rail2Thick, rail2Thick, balcony2Depth * 0.95f), 4.0f);
    addBox(vertices, indices, glm::vec3(balcony2Width * 0.48f, railY2 - rail2Height + 0.08f, 0),
           glm::vec3(rail2Thick, rail2Thick, balcony2Depth * 0.95f), 4.0f);
    
    // 欄杆垂直柱（間隔更大，避免與角柱重疊）
    float postSpacing2 = 1.8f;
    for (float x = -balcony2Width * 0.44f; x <= balcony2Width * 0.44f; x += postSpacing2) {
        // 避免與角柱重疊
        if (std::abs(x - col2X) > 0.4f && std::abs(x + col2X) > 0.4f) {
            addBox(vertices, indices, glm::vec3(x, railY2 - rail2Height * 0.5f + 0.04f, balcony2Depth * 0.48f),
                   glm::vec3(rail2Thick * 0.8f, rail2Height, rail2Thick * 0.8f), 4.0f);
            addBox(vertices, indices, glm::vec3(x, railY2 - rail2Height * 0.5f + 0.04f, -balcony2Depth * 0.48f),
                   glm::vec3(rail2Thick * 0.8f, rail2Height, rail2Thick * 0.8f), 4.0f);
        }
    }
    for (float z = -balcony2Depth * 0.35f; z <= balcony2Depth * 0.35f; z += postSpacing2) {
        // 避免與角柱重疊
        if (std::abs(z - col2Z) > 0.4f && std::abs(z + col2Z) > 0.4f) {
            addBox(vertices, indices, glm::vec3(-balcony2Width * 0.48f, railY2 - rail2Height * 0.5f + 0.04f, z),
                   glm::vec3(rail2Thick * 0.8f, rail2Height, rail2Thick * 0.8f), 4.0f);
            addBox(vertices, indices, glm::vec3(balcony2Width * 0.48f, railY2 - rail2Height * 0.5f + 0.04f, z),
                   glm::vec3(rail2Thick * 0.8f, rail2Height, rail2Thick * 0.8f), 4.0f);
        }
    }
    
    // 頂部橫樑
    float beam2Y = base2 + floor2Height;
    addBox(vertices, indices, glm::vec3(0, beam2Y, -col2Z),
           glm::vec3(floor2Width + 0.4f, 0.22f, 0.28f), 1.0f);
    addBox(vertices, indices, glm::vec3(0, beam2Y, col2Z),
           glm::vec3(floor2Width + 0.4f, 0.22f, 0.28f), 1.0f);
    
    // 窗戶（第二層前後牆 - 拱型）
    for (float x = -floor2Width * 0.3f; x <= floor2Width * 0.3f; x += floor2Width * 0.3f) {
        float winBaseY = base2 + floor2Height * 0.35f;
        float winWidth = 1.8f;
        float winRectHeight = 1.8f;  // 矩形部分高度
        float winArchHeight = 0.4f;  // 圓弧高度
        
        // 矩形底部
        addBox(vertices, indices, glm::vec3(x, winBaseY + winRectHeight * 0.5f, -floor2Depth * 0.46f),
               glm::vec3(winWidth, winRectHeight, 0.08f), 3.0f);
        addBox(vertices, indices, glm::vec3(x, winBaseY + winRectHeight * 0.5f, floor2Depth * 0.46f),
               glm::vec3(winWidth, winRectHeight, 0.08f), 3.0f);
        
        // 圓弧頂部（使用平滑圓弧）
        glm::vec3 archCenter1(x, winBaseY + winRectHeight, -floor2Depth * 0.46f);
        glm::vec3 archCenter2(x, winBaseY + winRectHeight, floor2Depth * 0.46f);
        addWindowArch(vertices, indices, archCenter1, winWidth, winArchHeight, 0.08f, 3.0f, false);
        addWindowArch(vertices, indices, archCenter2, winWidth, winArchHeight, 0.08f, 3.0f, false);
    }
    
    // 第二層左右牆窗戶（拱型）
    for (float z = -floor2Depth * 0.25f; z <= floor2Depth * 0.25f; z += floor2Depth * 0.5f) {
        float winBaseY = base2 + floor2Height * 0.35f;
        float winDepth = 1.6f;
        float winRectHeight = 1.7f;
        float winArchHeight = 0.35f;
        
        // 矩形底部
        addBox(vertices, indices, glm::vec3(-floor2Width * 0.48f, winBaseY + winRectHeight * 0.5f, z),
               glm::vec3(0.08f, winRectHeight, winDepth), 3.0f);
        addBox(vertices, indices, glm::vec3(floor2Width * 0.48f, winBaseY + winRectHeight * 0.5f, z),
               glm::vec3(0.08f, winRectHeight, winDepth), 3.0f);
        
        // 圓弧頂部（使用平滑圓弧）
        glm::vec3 archCenter3(-floor2Width * 0.48f, winBaseY + winRectHeight, z);
        glm::vec3 archCenter4(floor2Width * 0.48f, winBaseY + winRectHeight, z);
        addWindowArch(vertices, indices, archCenter3, winDepth, winArchHeight, 0.08f, 3.0f, true);
        addWindowArch(vertices, indices, archCenter4, winDepth, winArchHeight, 0.08f, 3.0f, true);
    }
    
    // 第二層屋頂
    float roof2Base = beam2Y + 0.12f;
    float roof2Height = 1.5f;
    float roof2Overhang = 2.0f;
    addRoof(vertices, indices, roof2Base, floor2Width + roof2Overhang * 2, 
            floor2Depth + roof2Overhang * 2, roof2Height, 18);
    
    // ======== 第三層（禪宗佛殿造 - 金色）========
    float base3 = roof2Base + roof2Height * 0.12f;
    
    // 金色牆面（四面）
    float wall3Y = base3 + floor3Height * 0.5f;
    addBox(vertices, indices, glm::vec3(0, wall3Y, -floor3Depth * 0.42f),
           glm::vec3(floor3Width * 0.9f, floor3Height, wallThick), 1.0f);
    addBox(vertices, indices, glm::vec3(0, wall3Y, floor3Depth * 0.42f),
           glm::vec3(floor3Width * 0.9f, floor3Height, wallThick), 1.0f);
    addBox(vertices, indices, glm::vec3(-floor3Width * 0.45f, wall3Y, 0),
           glm::vec3(wallThick, floor3Height, floor3Depth * 0.85f), 1.0f);
    addBox(vertices, indices, glm::vec3(floor3Width * 0.45f, wall3Y, 0),
           glm::vec3(wallThick, floor3Height, floor3Depth * 0.85f), 1.0f);
    
    // 地板
    addBox(vertices, indices, glm::vec3(0, base3, 0),
           glm::vec3(floor3Width, 0.1f, floor3Depth), 3.0f);
    
    // 角柱
    float col3X = floor3Width * 0.45f;
    float col3Z = floor3Depth * 0.42f;
    float colY3 = base3 + floor3Height * 0.5f;
    
    addBox(vertices, indices, glm::vec3(-col3X, colY3, -col3Z),
           glm::vec3(pillarRadius * 1.1f, floor3Height, pillarRadius * 1.1f), 1.0f);
    addBox(vertices, indices, glm::vec3(col3X, colY3, -col3Z),
           glm::vec3(pillarRadius * 1.1f, floor3Height, pillarRadius * 1.1f), 1.0f);
    addBox(vertices, indices, glm::vec3(-col3X, colY3, col3Z),
           glm::vec3(pillarRadius * 1.1f, floor3Height, pillarRadius * 1.1f), 1.0f);
    addBox(vertices, indices, glm::vec3(col3X, colY3, col3Z),
           glm::vec3(pillarRadius * 1.1f, floor3Height, pillarRadius * 1.1f), 1.0f);
    
    // ====== 第三層外推陽台欄杆 ======
    float balcony3Offset = 1.2f;
    float balcony3Width = floor3Width + balcony3Offset * 2;
    float balcony3Depth = floor3Depth + balcony3Offset * 2;
    float balcony3Floor = base3 - 0.08f;
    
    // 陽台地板
    addBox(vertices, indices, glm::vec3(0, balcony3Floor, 0),
           glm::vec3(balcony3Width, 0.08f, balcony3Depth), 3.0f);
    
    // 陽台欄杆（簡潔日式風格）
    float railY3 = balcony3Floor + 1.2f;
    float rail3Height = 0.55f;
    float rail3Thick = 0.045f;
    
    // 上下橫杆（四面）
    // 前後
    addBox(vertices, indices, glm::vec3(0, railY3, balcony3Depth * 0.48f),
           glm::vec3(balcony3Width * 0.95f, rail3Thick, rail3Thick), 4.0f);
    addBox(vertices, indices, glm::vec3(0, railY3, -balcony3Depth * 0.48f),
           glm::vec3(balcony3Width * 0.95f, rail3Thick, rail3Thick), 4.0f);
    addBox(vertices, indices, glm::vec3(0, railY3 - rail3Height + 0.06f, balcony3Depth * 0.48f),
           glm::vec3(balcony3Width * 0.95f, rail3Thick, rail3Thick), 4.0f);
    addBox(vertices, indices, glm::vec3(0, railY3 - rail3Height + 0.06f, -balcony3Depth * 0.48f),
           glm::vec3(balcony3Width * 0.95f, rail3Thick, rail3Thick), 4.0f);
    // 左右
    addBox(vertices, indices, glm::vec3(-balcony3Width * 0.48f, railY3, 0),
           glm::vec3(rail3Thick, rail3Thick, balcony3Depth * 0.95f), 4.0f);
    addBox(vertices, indices, glm::vec3(balcony3Width * 0.48f, railY3, 0),
           glm::vec3(rail3Thick, rail3Thick, balcony3Depth * 0.95f), 4.0f);
    addBox(vertices, indices, glm::vec3(-balcony3Width * 0.48f, railY3 - rail3Height + 0.06f, 0),
           glm::vec3(rail3Thick, rail3Thick, balcony3Depth * 0.95f), 4.0f);
    addBox(vertices, indices, glm::vec3(balcony3Width * 0.48f, railY3 - rail3Height + 0.06f, 0),
           glm::vec3(rail3Thick, rail3Thick, balcony3Depth * 0.95f), 4.0f);
    
    // 欄杆垂直柱（間隔更大，避免與角柱重疊）
    float postSpacing3 = 1.5f;
    for (float x = -balcony3Width * 0.42f; x <= balcony3Width * 0.42f; x += postSpacing3) {
        // 避免與角柱重疊
        if (std::abs(x - col3X) > 0.35f && std::abs(x + col3X) > 0.35f) {
            addBox(vertices, indices, glm::vec3(x, railY3 - rail3Height * 0.5f + 0.03f, balcony3Depth * 0.48f),
                   glm::vec3(rail3Thick * 0.8f, rail3Height, rail3Thick * 0.8f), 4.0f);
            addBox(vertices, indices, glm::vec3(x, railY3 - rail3Height * 0.5f + 0.03f, -balcony3Depth * 0.48f),
                   glm::vec3(rail3Thick * 0.8f, rail3Height, rail3Thick * 0.8f), 4.0f);
        }
    }
    for (float z = -balcony3Depth * 0.32f; z <= balcony3Depth * 0.32f; z += postSpacing3) {
        // 避免與角柱重疊
        if (std::abs(z - col3Z) > 0.35f && std::abs(z + col3Z) > 0.35f) {
            addBox(vertices, indices, glm::vec3(-balcony3Width * 0.48f, railY3 - rail3Height * 0.5f + 0.03f, z),
                   glm::vec3(rail3Thick * 0.8f, rail3Height, rail3Thick * 0.8f), 4.0f);
            addBox(vertices, indices, glm::vec3(balcony3Width * 0.48f, railY3 - rail3Height * 0.5f + 0.03f, z),
                   glm::vec3(rail3Thick * 0.8f, rail3Height, rail3Thick * 0.8f), 4.0f);
        }
    }
    
    // 頂部橫樑
    float beam3Y = base3 + floor3Height;
    addBox(vertices, indices, glm::vec3(0, beam3Y, 0),
           glm::vec3(floor3Width + 0.3f, 0.2f, floor3Depth + 0.3f), 1.0f);
    
    // 窗戶（第三層 - 拱型）
    float win3BaseY = base3 + floor3Height * 0.3f;
    float win3Width = 1.5f;
    float win3RectHeight = 1.6f;
    float win3ArchHeight = 0.35f;
    
    // 前後窗戶矩形底部
    addBox(vertices, indices, glm::vec3(0, win3BaseY + win3RectHeight * 0.5f, -floor3Depth * 0.44f),
           glm::vec3(win3Width, win3RectHeight, 0.06f), 3.0f);
    addBox(vertices, indices, glm::vec3(0, win3BaseY + win3RectHeight * 0.5f, floor3Depth * 0.44f),
           glm::vec3(win3Width, win3RectHeight, 0.06f), 3.0f);
    
    // 前後窗戶圓弧頂部（使用平滑圓弧）
    glm::vec3 arch3Center1(0, win3BaseY + win3RectHeight, -floor3Depth * 0.44f);
    glm::vec3 arch3Center2(0, win3BaseY + win3RectHeight, floor3Depth * 0.44f);
    addWindowArch(vertices, indices, arch3Center1, win3Width, win3ArchHeight, 0.06f, 3.0f, false);
    addWindowArch(vertices, indices, arch3Center2, win3Width, win3ArchHeight, 0.06f, 3.0f, false);
    
    // 第三層左右窗戶（拱型）
    for (float z = -floor3Depth * 0.2f; z <= floor3Depth * 0.2f; z += floor3Depth * 0.4f) {
        float win3Depth = 1.2f;
        float win3LRRectHeight = 1.5f;
        float win3LRArchHeight = 0.3f;
        
        // 矩形底部
        addBox(vertices, indices, glm::vec3(-floor3Width * 0.46f, win3BaseY + win3LRRectHeight * 0.5f, z),
               glm::vec3(0.06f, win3LRRectHeight, win3Depth), 3.0f);
        addBox(vertices, indices, glm::vec3(floor3Width * 0.46f, win3BaseY + win3LRRectHeight * 0.5f, z),
               glm::vec3(0.06f, win3LRRectHeight, win3Depth), 3.0f);
        
        // 圓弧頂部（使用平滑圓弧）
        glm::vec3 arch3Center3(-floor3Width * 0.46f, win3BaseY + win3LRRectHeight, z);
        glm::vec3 arch3Center4(floor3Width * 0.46f, win3BaseY + win3LRRectHeight, z);
        addWindowArch(vertices, indices, arch3Center3, win3Depth, win3LRArchHeight, 0.06f, 3.0f, true);
        addWindowArch(vertices, indices, arch3Center4, win3Depth, win3LRArchHeight, 0.06f, 3.0f, true);
    }
    
    // 第三層屋頂（最頂層）
    float roof3Base = beam3Y + 0.1f;
    float roof3Height = 1.8f;
    float roof3Overhang = 1.5f;
    addRoof(vertices, indices, roof3Base, floor3Width + roof3Overhang * 2, 
            floor3Depth + roof3Overhang * 2, roof3Height, 18);
    
    // 注意：已移除頂部金色鳳凰裝飾
    
    // ======== 兩側木棧板走廊（延伸到陸地）========
    float walkwayWidth = 3.5f;        // 走廊寬度
    float walkwayLength = 18.0f;      // 走廊長度（延伸距離）
    float walkwayHeight = 0.2f;       // 地板厚度
    float walkwayFloorY = base1;      // 與第一層同高
    float corridorHeight = 2.8f;      // 走廊高度
    
    // 左側木棧板走廊
    float leftWalkwayX = -floor1Width * 0.5f - walkwayWidth * 0.5f;
    
    // 左側地板
    addBox(vertices, indices, glm::vec3(leftWalkwayX, walkwayFloorY, 0),
           glm::vec3(walkwayWidth, walkwayHeight, walkwayLength), 3.0f);
    
    // 左側支撐柱（多根柱子）
    for (float z = -walkwayLength * 0.4f; z <= walkwayLength * 0.4f; z += 3.0f) {
        float pillarY = walkwayFloorY + corridorHeight * 0.5f;
        addBox(vertices, indices, glm::vec3(leftWalkwayX - walkwayWidth * 0.4f, pillarY, z),
               glm::vec3(0.2f, corridorHeight, 0.2f), 0.0f);
        addBox(vertices, indices, glm::vec3(leftWalkwayX + walkwayWidth * 0.4f, pillarY, z),
               glm::vec3(0.2f, corridorHeight, 0.2f), 0.0f);
    }
    
    // 左側屋頂橫樑
    float walkwayRoofY = walkwayFloorY + corridorHeight;
    addBox(vertices, indices, glm::vec3(leftWalkwayX, walkwayRoofY, 0),
           glm::vec3(walkwayWidth + 0.3f, 0.15f, walkwayLength + 0.3f), 0.0f);
    
    // 左側屋頂（簡化版，3層）
    for (int i = 0; i < 3; i++) {
        float roofY = walkwayRoofY + 0.1f + i * 0.15f;
        float roofScale = 1.0f - i * 0.15f;
        addBox(vertices, indices, glm::vec3(leftWalkwayX, roofY, 0),
               glm::vec3((walkwayWidth + 1.0f) * roofScale, 0.12f, 
                        (walkwayLength + 1.0f) * roofScale), 2.0f);
    }
    
    // 右側木棧板走廊
    float rightWalkwayX = floor1Width * 0.5f + walkwayWidth * 0.5f;
    
    // 右側地板
    addBox(vertices, indices, glm::vec3(rightWalkwayX, walkwayFloorY, 0),
           glm::vec3(walkwayWidth, walkwayHeight, walkwayLength), 3.0f);
    
    // 右側支撐柱（多根柱子）
    for (float z = -walkwayLength * 0.4f; z <= walkwayLength * 0.4f; z += 3.0f) {
        float pillarY = walkwayFloorY + corridorHeight * 0.5f;
        addBox(vertices, indices, glm::vec3(rightWalkwayX - walkwayWidth * 0.4f, pillarY, z),
               glm::vec3(0.2f, corridorHeight, 0.2f), 0.0f);
        addBox(vertices, indices, glm::vec3(rightWalkwayX + walkwayWidth * 0.4f, pillarY, z),
               glm::vec3(0.2f, corridorHeight, 0.2f), 0.0f);
    }
    
    // 右側屋頂橫樑
    addBox(vertices, indices, glm::vec3(rightWalkwayX, walkwayRoofY, 0),
           glm::vec3(walkwayWidth + 0.3f, 0.15f, walkwayLength + 0.3f), 0.0f);
    
    // 右側屋頂（簡化版，3層）
    for (int i = 0; i < 3; i++) {
        float roofY = walkwayRoofY + 0.1f + i * 0.15f;
        float roofScale = 1.0f - i * 0.15f;
        addBox(vertices, indices, glm::vec3(rightWalkwayX, roofY, 0),
               glm::vec3((walkwayWidth + 1.0f) * roofScale, 0.12f, 
                        (walkwayLength + 1.0f) * roofScale), 2.0f);
    }
    
    // 上傳到GPU
    indexCount = indices.size();
    
    glGenVertexArrays(1, &houseVAO);
    glGenBuffers(1, &houseVBO);
    glGenBuffers(1, &houseEBO);
    
    glBindVertexArray(houseVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, houseVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), 
                 vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, houseEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), 
                 indices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), 
                         (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(float), 
                         (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
    
    shaderProgram = loadShader("../shaders/house.vert", "../shaders/house.frag");
    
    std::cout << "[Kinkaku-ji] Initialized with " << indexCount << " indices" << std::endl;
}

void House::render(const glm::mat4& view, const glm::mat4& projection,
                     std::vector<Light> lights, Light env_light, 
                   const glm::vec3& viewPos, float time) {
    lights.push_back(env_light);
       glUseProgram(shaderProgram);
    
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotationY), glm::vec3(0, 1, 0));
    model = glm::scale(model, glm::vec3(scale));
    
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
    
    int numLights = lights.size();
    glUniform1i(glGetUniformLocation(shaderProgram, "numLights"), numLights);
    for (int i = 0; i < numLights && i < 100; ++i) {
        std::string base = "lights[" + std::to_string(i) + "]";
        glUniform3fv(glGetUniformLocation(shaderProgram, (base + ".position").c_str()), 1, glm::value_ptr(lights[i].position));
        glUniform3fv(glGetUniformLocation(shaderProgram, (base + ".color").c_str()), 1, glm::value_ptr(lights[i].color));
        glUniform1f(glGetUniformLocation(shaderProgram, (base + ".intensity").c_str()), lights[i].intensity);
    }
    
    glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, &viewPos[0]);
    glUniform1f(glGetUniformLocation(shaderProgram, "time"), time);
    
    glBindVertexArray(houseVAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
