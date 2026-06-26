#include "Tree.h"

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <random>

static std::random_device rd;
static std::mt19937 gen(rd());

static float randomFloat(float min, float max) {
    std::uniform_real_distribution<float> dis(min, max);
    return dis(gen);
}

static std::string readFile(const char* path) {
    std::ifstream ifs(path);
    std::stringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

static GLuint compileShader(GLenum type, const std::string& src) {
    GLuint s = glCreateShader(type);
    const char* c = src.c_str();
    glShaderSource(s, 1, &c, nullptr);
    glCompileShader(s);

    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetShaderInfoLog(s, len, &len, log.data());
        std::cerr << "[Tree] Shader compile error:\n" << log << "\n";
        glDeleteShader(s);
        return 0;
    }
    return s;
}

GLuint Tree::loadShader(const char* vertexPath, const char* fragmentPath) {
    std::string vertexCode, fragmentCode;
    std::ifstream vShaderFile, fShaderFile;
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    
    try {
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        vShaderFile.close();
        fShaderFile.close();
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    } catch (std::ifstream::failure& e) {
        std::cerr << "ERROR::SHADER::FILE_NOT_READ: " << e.what() << std::endl;
        return 0;
    }
    
    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();
    GLuint vertex, fragment;
    GLint success;
    GLchar infoLog[512];
    
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return program;
}

// 添加橫向展開的樹枝（水平或微微向上生長，不下垂）
void Tree::addHorizontalBranch(const glm::vec3& start, const glm::vec3& direction, 
                                float length, float radius, bool addLeaves) {
    // 生成曲線樹枝
    glm::vec3 perpY(0, 1, 0);
    glm::vec3 perpH = glm::normalize(glm::cross(direction, perpY));
    
    // 分段生成樹枝
    int numSegments = 4;
    glm::vec3 currentPos = start;
    glm::vec3 currentDir = direction;
    float segmentLength = length / numSegments;
    float currentRadius = radius;
    
    std::vector<glm::vec3> branchPoints;
    branchPoints.push_back(currentPos);
    
    for (int i = 0; i < numSegments; i++) {
        // 保持水平或微微向上（不下垂！）
        float lift = randomFloat(0.0f, 0.08f);  // 只向上，不向下
        float sway = randomFloat(-0.06f, 0.06f);
        
        glm::vec3 offset(sway, lift, sway);
        glm::vec3 nextPos = currentPos + currentDir * segmentLength + offset;
        branchPoints.push_back(nextPos);
        
        TreeBranch branch;
        branch.start = currentPos;
        branch.end = nextPos;
        branch.radius = currentRadius;
        branch.level = 2;
        
        glm::vec3 midPoint = (currentPos + nextPos) * 0.5f;
        midPoint.y += randomFloat(0.0f, 0.1f);  // 中點也只向上
        branch.curvePoints = {currentPos, midPoint, nextPos};
        branches.push_back(branch);
        
        currentPos = nextPos;
        currentRadius *= 0.75f;
    }
    
    // 在樹枝末端和中間添加更多葉子
    if (addLeaves) {
        glm::vec3 tipPos = branchPoints.back();
        addLeafLayer(tipPos, direction, 3.0f);  // 末端較大的葉子團
        
        // 在樹枝中段也添加葉子
        if (branchPoints.size() >= 3) {
            glm::vec3 midPos = branchPoints[branchPoints.size() / 2];
            addLeafLayer(midPos, direction, 2.0f);  // 中段較小的葉子團
        }
    }
}

// 添加分層的葉子（不規則形狀的葉子團）
void Tree::addLeafLayer(const glm::vec3& center, const glm::vec3& branchDir, float radius) {
    // 創建自然的不規則圓形葉子團
    // 使用多個隨機橢圓來創建不規則邊界
    
    // 創建 4-6 個立體層
    int numLayers = 4 + (int)randomFloat(0, 3);
    
    // 為這個葉子團生成隨機的不規則形狀參數
    int numBlobs = 4 + (int)randomFloat(0, 3);
    struct Blob {
        float angle;
        float radiusScale;
    };
    std::vector<Blob> blobs;
    for (int i = 0; i < numBlobs; i++) {
        Blob b;
        b.angle = i * 2.0f * 3.14159f / numBlobs + randomFloat(-0.3f, 0.3f);
        b.radiusScale = randomFloat(0.7f, 1.3f);
        blobs.push_back(b);
    }
    
    for (int layer = 0; layer < numLayers; layer++) {
        float layerHeight = layer * 0.5f;
        int leavesInLayer = 70 + (int)randomFloat(0, 35);
        
        for (int i = 0; i < leavesInLayer; i++) {
            LeafParticle leaf;
            
            // 使用極坐標生成圓形分布，添加不規則性
            float angle = randomFloat(0, 2 * 3.14159f);
            
            // 根據角度計算不規則半徑（使用blob來創建不規則形狀）
            float irregularRadius = radius;
            for (const auto& blob : blobs) {
                float angleDiff = angle - blob.angle;
                // 使用柔和的權重函數
                float weight = (cos(angleDiff) + 1.0f) * 0.5f;
                weight = pow(weight, 2.0f);  // 平方使過渡更柔和
                irregularRadius += (blob.radiusScale - 1.0f) * radius * 0.3f * weight;
            }
            
            // 徑向距離使用非均勻分布，讓邊緣更密集
            float r = pow(randomFloat(0.2f, 1.0f), 0.7f) * irregularRadius;
            
            // 添加柏林噪聲式的擾動
            float noiseScale = 0.3f;
            float noise = sin(angle * 3.0f) * cos(angle * 2.0f) * noiseScale;
            r *= (1.0f + noise * randomFloat(0.5f, 1.0f));
            
            float baseX = r * cos(angle);
            float baseZ = r * sin(angle);
            
            // 添加細微的隨機擾動
            baseX += randomFloat(-0.15f, 0.15f);
            baseZ += randomFloat(-0.15f, 0.15f);
            
            // 更大的高度變化，讓葉子團更立體
            float heightVar = randomFloat(-0.4f, 0.5f);
            
            leaf.position = center + glm::vec3(baseX, layerHeight + heightVar, baseZ);
            
            // 兩種紅色：深紅和亮紅
            bool isDarkRed = randomFloat(0, 1) < 0.5f;
            float colorVar = randomFloat(0.9f, 1.0f);
            
            if (isDarkRed) {
                leaf.color = glm::vec3(0.6f * colorVar, 0.05f * colorVar, 0.08f * colorVar);
            } else {
                leaf.color = glm::vec3(0.95f * colorVar, 0.15f * colorVar, 0.05f * colorVar);
            }
            
            leaf.size = randomFloat(0.2f, 0.4f);
            leaf.rotation = randomFloat(0, 2 * 3.14159f);
            
            leafParticles.push_back(leaf);
        }
    }
    
    // 在中心區域也添加一些葉子（更立體）
    int centerLeaves = 50 + (int)randomFloat(0, 25);
    for (int i = 0; i < centerLeaves; i++) {
        LeafParticle leaf;
        float angle = randomFloat(0, 2 * 3.14159f);
        float r = randomFloat(0.1f, 0.5f) * radius * 0.5f;
        
        leaf.position = center + glm::vec3(
            r * cos(angle),
            randomFloat(0, 1.8f),  // 更高的垂直分布
            r * sin(angle)
        );
        
        bool isDarkRed = randomFloat(0, 1) < 0.5f;
        float colorVar = randomFloat(0.9f, 1.0f);
        if (isDarkRed) {
            leaf.color = glm::vec3(0.6f * colorVar, 0.05f * colorVar, 0.08f * colorVar);
        } else {
            leaf.color = glm::vec3(0.95f * colorVar, 0.15f * colorVar, 0.05f * colorVar);
        }
        
        leaf.size = randomFloat(0.2f, 0.4f);
        leaf.rotation = randomFloat(0, 2 * 3.14159f);
        leafParticles.push_back(leaf);
    }
}

// 添加扁平分布的葉子團（用於頂部，不規則形狀）
void Tree::addLeafClusterFlat(const glm::vec3& center, float radius, int count) {
    // 創建自然的不規則圓形葉子團（頂部扁平版本）
    
    // 為這個葉子團生成隨機的不規則形狀參數
    int numBlobs = 5 + (int)randomFloat(0, 3);
    struct Blob {
        float angle;
        float radiusScale;
    };
    std::vector<Blob> blobs;
    for (int i = 0; i < numBlobs; i++) {
        Blob b;
        b.angle = i * 2.0f * 3.14159f / numBlobs + randomFloat(-0.4f, 0.4f);
        b.radiusScale = randomFloat(0.6f, 1.4f);
        blobs.push_back(b);
    }
    
    for (int i = 0; i < count; i++) {
        LeafParticle leaf;
        
        // 使用極坐標生成圓形分布
        float angle = randomFloat(0, 2 * 3.14159f);
        
        // 根據角度計算不規則半徑
        float irregularRadius = radius;
        for (const auto& blob : blobs) {
            float angleDiff = angle - blob.angle;
            // 使用柔和的權重函數
            float weight = (cos(angleDiff) + 1.0f) * 0.5f;
            weight = pow(weight, 1.5f);
            irregularRadius += (blob.radiusScale - 1.0f) * radius * 0.35f * weight;
        }
        
        // 徑向距離（邊緣更密集）
        float r = pow(randomFloat(0.15f, 1.0f), 0.65f) * irregularRadius;
        
        // 添加多頻率的擾動
        float noise = sin(angle * 2.5f) * cos(angle * 1.7f) * 0.25f +
                      sin(angle * 5.0f) * 0.1f;
        r *= (1.0f + noise * randomFloat(0.6f, 1.0f));
        
        float x = r * cos(angle);
        float z = r * sin(angle);
        
        // 添加細微的隨機擾動
        x += randomFloat(-0.12f, 0.12f);
        z += randomFloat(-0.12f, 0.12f);
        
        // 更大的高度變化，使葉子團更立體
        float heightVar = randomFloat(-0.3f, 0.6f);
        
        leaf.position = center + glm::vec3(x, heightVar, z);
        
        // 兩種紅色
        bool isDarkRed = randomFloat(0, 1) < 0.45f;
        float colorVar = randomFloat(0.9f, 1.0f);
        
        if (isDarkRed) {
            leaf.color = glm::vec3(0.55f * colorVar, 0.04f * colorVar, 0.07f * colorVar);
        } else {
            leaf.color = glm::vec3(0.92f * colorVar, 0.12f * colorVar, 0.04f * colorVar);
        }
        
        leaf.size = randomFloat(0.18f, 0.38f);
        leaf.rotation = randomFloat(0, 2 * 3.14159f);
        
        leafParticles.push_back(leaf);
    }
    
    // 中心區域葉子（更立體）
    int centerLeaves = 30 + (int)randomFloat(0, 20);
    for (int i = 0; i < centerLeaves; i++) {
        LeafParticle leaf;
        float angle = randomFloat(0, 2 * 3.14159f);
        float r = randomFloat(0.05f, 0.3f) * radius;
        
        leaf.position = center + glm::vec3(r * cos(angle), randomFloat(0, 0.8f), r * sin(angle));
        
        bool isDarkRed = randomFloat(0, 1) < 0.5f;
        float colorVar = randomFloat(0.9f, 1.0f);
        if (isDarkRed) {
            leaf.color = glm::vec3(0.55f * colorVar, 0.04f * colorVar, 0.07f * colorVar);
        } else {
            leaf.color = glm::vec3(0.92f * colorVar, 0.12f * colorVar, 0.04f * colorVar);
        }
        
        leaf.size = randomFloat(0.18f, 0.35f);
        leaf.rotation = randomFloat(0, 2 * 3.14159f);
        leafParticles.push_back(leaf);
    }
}

// 遞歸生成樹枝（用於主幹分枝）
void Tree::generateBranch(const glm::vec3& start, const glm::vec3& direction, 
                          float length, float radius, int level, int maxLevel) {
    // 此函數保留用於兼容性，主要邏輯在 generateBranches 中
}

void Tree::generateBranches() {
    branches.clear();
    leafParticles.clear();
    
    // 隨機選擇樹形風格 (0-4)
    int treeStyle = (int)randomFloat(0, 4.99f);
    
    float trunkRadius = 0.6f;
    std::vector<glm::vec3> trunkPoints;
    
    // 5種橫向彎曲但稍高的樹幹
    switch (treeStyle) {
        case 0:  // 風格1: 強烈S型橫向彎曲
            trunkPoints = {
                glm::vec3(0, 0, 0),
                glm::vec3(0.7f, 0.9f, 0.2f),
                glm::vec3(1.3f, 1.8f, -0.3f),
                glm::vec3(0.8f, 2.8f, 0.4f),
                glm::vec3(1.4f, 3.6f, 0.1f),
                glm::vec3(1.0f, 4.3f, -0.2f)
            };
            break;
            
        case 1:  // 風格2: 右傾橫臥
            trunkPoints = {
                glm::vec3(0, 0, 0),
                glm::vec3(1.0f, 0.8f, 0.1f),
                glm::vec3(1.7f, 1.6f, -0.2f),
                glm::vec3(2.1f, 2.5f, 0.3f),
                glm::vec3(2.4f, 3.4f, -0.1f),
                glm::vec3(2.0f, 4.2f, 0.2f)
            };
            break;
            
        case 2:  // 風格3: 前傾蜿蜒
            trunkPoints = {
                glm::vec3(0, 0, 0),
                glm::vec3(0.3f, 0.8f, 0.8f),
                glm::vec3(-0.1f, 1.7f, 1.4f),
                glm::vec3(0.4f, 2.6f, 1.8f),
                glm::vec3(0.0f, 3.5f, 2.1f),
                glm::vec3(0.3f, 4.3f, 1.7f)
            };
            break;
            
        case 3:  // 風格4: 左傾扭轉
            trunkPoints = {
                glm::vec3(0, 0, 0),
                glm::vec3(-0.8f, 0.8f, 0.4f),
                glm::vec3(-1.3f, 1.7f, -0.3f),
                glm::vec3(-0.9f, 2.7f, 0.5f),
                glm::vec3(-1.5f, 3.5f, 0.1f),
                glm::vec3(-1.1f, 4.3f, -0.2f)
            };
            break;
            
        case 4:  // 風格5: 後傾彎曲
        default:
            trunkPoints = {
                glm::vec3(0, 0, 0),
                glm::vec3(0.4f, 0.9f, -0.7f),
                glm::vec3(-0.1f, 1.8f, -1.2f),
                glm::vec3(0.4f, 2.7f, -1.5f),
                glm::vec3(0.0f, 3.5f, -1.3f),
                glm::vec3(0.3f, 4.3f, -1.6f)
            };
            break;
    }
    
    // 添加隨機擾動
    for (size_t i = 1; i < trunkPoints.size(); i++) {
        trunkPoints[i].x += randomFloat(-0.2f, 0.2f);
        trunkPoints[i].z += randomFloat(-0.2f, 0.2f);
    }
    
    // 生成主幹段
    for (size_t i = 0; i < trunkPoints.size() - 1; i++) {
        TreeBranch trunk;
        trunk.start = trunkPoints[i];
        trunk.end = trunkPoints[i + 1];
        trunk.radius = trunkRadius * (1.0f - (float)i * 0.12f);
        trunk.level = 0;
        
        glm::vec3 mid = (trunkPoints[i] + trunkPoints[i + 1]) * 0.5f;
        mid.x += randomFloat(-0.25f, 0.25f);
        mid.z += randomFloat(-0.25f, 0.25f);
        trunk.curvePoints = {trunkPoints[i], mid, trunkPoints[i + 1]};
        branches.push_back(trunk);
    }
    
    // 根據樹形風格生成不對稱的分支
    generateBranchesForStyle(treeStyle, trunkPoints, trunkRadius);
    
    std::cout << "[Tree] Style " << treeStyle << ": Generated " << branches.size() 
              << " branches and " << leafParticles.size() << " leaf particles" << std::endl;
}

void Tree::generateBranchesForStyle(int style, const std::vector<glm::vec3>& trunkPoints, float trunkRadius) {
    // 分支配置：不對稱，數量為(1,2)、(3,2)或(1,1)
    // 方向y分量 >= 0（不下垂）
    
    switch (style) {
        case 0:  // S型 - 左1右2
            // 右側2條
            addHorizontalBranch(trunkPoints[2], glm::normalize(glm::vec3(1.0f, 0.1f, 0.3f)), 
                                4.0f, trunkRadius * 0.5f, true);
            addHorizontalBranch(trunkPoints[4], glm::normalize(glm::vec3(0.9f, 0.15f, -0.4f)), 
                                3.5f, trunkRadius * 0.42f, true);
            // 左側1條
            addHorizontalBranch(trunkPoints[3], glm::normalize(glm::vec3(-0.95f, 0.08f, 0.3f)), 
                                3.8f, trunkRadius * 0.45f, true);
            break;
            
        case 1:  // 右傾 - 左1右2
            // 右側2條（主要方向）
            addHorizontalBranch(trunkPoints[2], glm::normalize(glm::vec3(0.95f, 0.05f, 0.3f)), 
                                4.5f, trunkRadius * 0.5f, true);
            addHorizontalBranch(trunkPoints[4], glm::normalize(glm::vec3(0.85f, 0.2f, -0.5f)), 
                                3.8f, trunkRadius * 0.4f, true);
            // 左側1條
            addHorizontalBranch(trunkPoints[3], glm::normalize(glm::vec3(-0.8f, 0.12f, 0.6f)), 
                                3.0f, trunkRadius * 0.38f, true);
            break;
            
        case 2:  // 前傾 - 左3右2
            // 右側2條
            addHorizontalBranch(trunkPoints[2], glm::normalize(glm::vec3(0.9f, 0.1f, 0.45f)), 
                                3.8f, trunkRadius * 0.48f, true);
            addHorizontalBranch(trunkPoints[4], glm::normalize(glm::vec3(0.7f, 0.18f, -0.7f)), 
                                3.2f, trunkRadius * 0.38f, true);
            // 左側3條
            addHorizontalBranch(trunkPoints[2], glm::normalize(glm::vec3(-0.85f, 0.05f, 0.5f)), 
                                4.2f, trunkRadius * 0.5f, true);
            addHorizontalBranch(trunkPoints[3], glm::normalize(glm::vec3(-0.9f, 0.12f, -0.4f)), 
                                3.5f, trunkRadius * 0.42f, true);
            addHorizontalBranch(trunkPoints[4], glm::normalize(glm::vec3(-0.75f, 0.2f, 0.65f)), 
                                3.0f, trunkRadius * 0.35f, true);
            break;
            
        case 3:  // 左傾 - 左1右1
            // 右側1條
            addHorizontalBranch(trunkPoints[3], glm::normalize(glm::vec3(0.9f, 0.1f, 0.45f)), 
                                4.0f, trunkRadius * 0.48f, true);
            // 左側1條
            addHorizontalBranch(trunkPoints[4], glm::normalize(glm::vec3(-0.85f, 0.15f, -0.5f)), 
                                3.5f, trunkRadius * 0.42f, true);
            break;
            
        case 4:  // 後傾 - 左2右3
        default:
            // 右側3條
            addHorizontalBranch(trunkPoints[2], glm::normalize(glm::vec3(0.95f, 0.08f, 0.3f)), 
                                4.0f, trunkRadius * 0.5f, true);
            addHorizontalBranch(trunkPoints[3], glm::normalize(glm::vec3(0.8f, 0.12f, -0.6f)), 
                                3.5f, trunkRadius * 0.42f, true);
            addHorizontalBranch(trunkPoints[4], glm::normalize(glm::vec3(0.7f, 0.2f, 0.7f)), 
                                3.0f, trunkRadius * 0.35f, true);
            // 左側2條
            addHorizontalBranch(trunkPoints[2], glm::normalize(glm::vec3(-0.9f, 0.05f, -0.45f)), 
                                3.8f, trunkRadius * 0.45f, true);
            addHorizontalBranch(trunkPoints[4], glm::normalize(glm::vec3(-0.75f, 0.18f, 0.65f)), 
                                3.2f, trunkRadius * 0.38f, true);
            break;
    }
    
    // 在頂部添加葉子團
    addLeafClusterFlat(trunkPoints.back() + glm::vec3(0, 0.3f, 0), 2.0f, 100);
}

void Tree::addLeafCluster(const glm::vec3& center, float radius, int count) {
    for (int i = 0; i < count; i++) {
        LeafParticle leaf;
        
        // 在球體內隨機分布
        float theta = randomFloat(0, 2 * 3.14159f);
        float phi = randomFloat(0, 3.14159f);
        float r = randomFloat(0.3f, 1.0f) * radius;
        
        leaf.position = center + glm::vec3(
            r * sin(phi) * cos(theta),
            r * cos(phi),
            r * sin(phi) * sin(theta)
        );
        
        // 紅色系顏色變化
        float colorVar = randomFloat(0.8f, 1.0f);
        leaf.color = glm::vec3(
            0.9f * colorVar,
            0.15f * colorVar,
            0.1f * colorVar
        );
        
        leaf.size = randomFloat(0.4f, 0.8f);  // 更大的葉子粒子
        leaf.rotation = randomFloat(0, 2 * 3.14159f);
        
        leafParticles.push_back(leaf);
    }
}

void Tree::generateLeafParticles() {
    leafParticles.clear();
    // 葉子已在 generateBranches 中生成
}

// 生成圓柱體幾何
void Tree::addCylinder(std::vector<float>& vertices, std::vector<unsigned int>& indices,
                       const glm::vec3& start, const glm::vec3& end, 
                       float radiusStart, float radiusEnd, int segments) {
    glm::vec3 direction = end - start;
    float length = glm::length(direction);
    if (length < 0.001f) return;
    
    direction = glm::normalize(direction);
    
    // 找到垂直於方向的兩個正交向量
    glm::vec3 up = (abs(direction.y) < 0.99f) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
    glm::vec3 right = glm::normalize(glm::cross(up, direction));
    up = glm::normalize(glm::cross(direction, right));
    
    unsigned int baseIndex = vertices.size() / 6;
    
    // 生成圓柱體頂點
    for (int ring = 0; ring <= 1; ring++) {
        float t = (float)ring;
        glm::vec3 center = start + direction * length * t;
        float radius = radiusStart + (radiusEnd - radiusStart) * t;
        
        for (int i = 0; i <= segments; i++) {
            float angle = (float)i / segments * 2.0f * 3.14159f;
            float x = cos(angle);
            float z = sin(angle);
            
            glm::vec3 pos = center + (right * x + up * z) * radius;
            glm::vec3 normal = glm::normalize(right * x + up * z);
            
            vertices.push_back(pos.x);
            vertices.push_back(pos.y);
            vertices.push_back(pos.z);
            vertices.push_back(normal.x);
            vertices.push_back(normal.y);
            vertices.push_back(normal.z);
        }
    }
    
    // 生成索引
    for (int i = 0; i < segments; i++) {
        unsigned int i0 = baseIndex + i;
        unsigned int i1 = baseIndex + i + 1;
        unsigned int i2 = baseIndex + segments + 1 + i;
        unsigned int i3 = baseIndex + segments + 1 + i + 1;
        
        indices.push_back(i0);
        indices.push_back(i2);
        indices.push_back(i1);
        
        indices.push_back(i1);
        indices.push_back(i2);
        indices.push_back(i3);
    }
}

void Tree::generateBranchGeometry(std::vector<float>& vertices, std::vector<unsigned int>& indices) {
    const int segments = 12;  // 更多段數使圓柱更圓滑
    
    for (const auto& branch : branches) {
        // 使用更多的插值點來平滑曲線
        std::vector<glm::vec3> smoothPoints;
        std::vector<float> smoothRadii;
        
        // 對曲線進行細分插值
        int subdivisions = 4;  // 每段細分為4個小段
        for (size_t i = 0; i < branch.curvePoints.size() - 1; i++) {
            for (int s = 0; s < subdivisions; s++) {
                float t = (float)s / subdivisions;
                glm::vec3 p0 = branch.curvePoints[i];
                glm::vec3 p1 = branch.curvePoints[i + 1];
                
                // 如果有中點，使用二次曲線插值
                glm::vec3 point;
                if (branch.curvePoints.size() == 3 && i == 0) {
                    // 使用二次貝塞爾曲線
                    float totalT = (i + t) / (branch.curvePoints.size() - 1);
                    glm::vec3 cp0 = branch.curvePoints[0];
                    glm::vec3 cp1 = branch.curvePoints[1];
                    glm::vec3 cp2 = branch.curvePoints[2];
                    point = (1-totalT)*(1-totalT)*cp0 + 2*(1-totalT)*totalT*cp1 + totalT*totalT*cp2;
                } else {
                    point = glm::mix(p0, p1, t);
                }
                
                smoothPoints.push_back(point);
                
                // 計算對應的半徑（逐漸變細）
                float overallT = ((float)i + t) / (branch.curvePoints.size() - 1);
                float r = branch.radius * (1.0f - overallT * 0.35f);
                smoothRadii.push_back(r);
            }
        }
        // 添加最後一個點
        smoothPoints.push_back(branch.curvePoints.back());
        smoothRadii.push_back(branch.radius * 0.65f);
        
        // 生成連續的管狀幾何體
        addTube(vertices, indices, smoothPoints, smoothRadii, segments);
        
        // 在分支起點添加球形節點來遮蓋連接處
        if (branch.level > 0) {
            addSphere(vertices, indices, branch.start, branch.radius * 1.1f, 8);
        }
    }
}

// 生成連續的管狀幾何體（無縫隙）
void Tree::addTube(std::vector<float>& vertices, std::vector<unsigned int>& indices,
                   const std::vector<glm::vec3>& points, const std::vector<float>& radii, int segments) {
    if (points.size() < 2) return;
    
    unsigned int baseIndex = vertices.size() / 6;
    
    // 為每個點生成一圈頂點
    for (size_t p = 0; p < points.size(); p++) {
        glm::vec3 pos = points[p];
        float radius = radii[p];
        
        // 計算此處的方向
        glm::vec3 direction;
        if (p == 0) {
            direction = glm::normalize(points[1] - points[0]);
        } else if (p == points.size() - 1) {
            direction = glm::normalize(points[p] - points[p-1]);
        } else {
            direction = glm::normalize(points[p+1] - points[p-1]);
        }
        
        // 找到正交基底
        glm::vec3 up = (abs(direction.y) < 0.99f) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
        glm::vec3 right = glm::normalize(glm::cross(up, direction));
        up = glm::normalize(glm::cross(direction, right));
        
        // 生成一圈頂點
        for (int i = 0; i <= segments; i++) {
            float angle = (float)i / segments * 2.0f * 3.14159f;
            float x = cos(angle);
            float z = sin(angle);
            
            glm::vec3 vertPos = pos + (right * x + up * z) * radius;
            glm::vec3 normal = glm::normalize(right * x + up * z);
            
            vertices.push_back(vertPos.x);
            vertices.push_back(vertPos.y);
            vertices.push_back(vertPos.z);
            vertices.push_back(normal.x);
            vertices.push_back(normal.y);
            vertices.push_back(normal.z);
        }
    }
    
    // 生成索引（連接相鄰的圈）
    int vertsPerRing = segments + 1;
    for (size_t p = 0; p < points.size() - 1; p++) {
        for (int i = 0; i < segments; i++) {
            unsigned int i0 = baseIndex + p * vertsPerRing + i;
            unsigned int i1 = baseIndex + p * vertsPerRing + i + 1;
            unsigned int i2 = baseIndex + (p + 1) * vertsPerRing + i;
            unsigned int i3 = baseIndex + (p + 1) * vertsPerRing + i + 1;
            
            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i1);
            
            indices.push_back(i1);
            indices.push_back(i2);
            indices.push_back(i3);
        }
    }
}

// 在連接處添加球形節點
void Tree::addSphere(std::vector<float>& vertices, std::vector<unsigned int>& indices,
                     const glm::vec3& center, float radius, int detail) {
    unsigned int baseIndex = vertices.size() / 6;
    
    // 生成球面頂點
    for (int lat = 0; lat <= detail; lat++) {
        float theta = (float)lat / detail * 3.14159f;
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);
        
        for (int lon = 0; lon <= detail; lon++) {
            float phi = (float)lon / detail * 2.0f * 3.14159f;
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);
            
            glm::vec3 normal(sinTheta * cosPhi, cosTheta, sinTheta * sinPhi);
            glm::vec3 pos = center + normal * radius;
            
            vertices.push_back(pos.x);
            vertices.push_back(pos.y);
            vertices.push_back(pos.z);
            vertices.push_back(normal.x);
            vertices.push_back(normal.y);
            vertices.push_back(normal.z);
        }
    }
    
    // 生成索引
    int vertsPerRow = detail + 1;
    for (int lat = 0; lat < detail; lat++) {
        for (int lon = 0; lon < detail; lon++) {
            unsigned int i0 = baseIndex + lat * vertsPerRow + lon;
            unsigned int i1 = baseIndex + lat * vertsPerRow + lon + 1;
            unsigned int i2 = baseIndex + (lat + 1) * vertsPerRow + lon;
            unsigned int i3 = baseIndex + (lat + 1) * vertsPerRow + lon + 1;
            
            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i1);
            
            indices.push_back(i1);
            indices.push_back(i2);
            indices.push_back(i3);
        }
    }
}

void Tree::initialize() {
    // 生成樹枝和葉子
    generateBranches();  // 這個函數會同時生成葉子
    
    std::cout << "[Tree] After generation: " << leafParticles.size() << " leaves" << std::endl;
    
    // ===== 樹幹/樹枝幾何 =====
    std::vector<float> branchVertices;
    std::vector<unsigned int> branchIndices;
    generateBranchGeometry(branchVertices, branchIndices);
    
    branchIndexCount = (int)branchIndices.size();
    
    glGenVertexArrays(1, &branchVAO);
    glGenBuffers(1, &branchVBO);
    glGenBuffers(1, &branchEBO);
    
    glBindVertexArray(branchVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, branchVBO);
    glBufferData(GL_ARRAY_BUFFER, branchVertices.size() * sizeof(float), 
                 branchVertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, branchEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, branchIndices.size() * sizeof(unsigned int),
                 branchIndices.data(), GL_STATIC_DRAW);
    
    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
    
    // ===== 葉子 Billboard =====
    // 單個 quad 頂點
    float quadVertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.5f,  0.5f, 0.0f,
        -0.5f,  0.5f, 0.0f
    };
    
    unsigned int quadIndices[] = {
        0, 1, 2,
        0, 2, 3
    };
    
    glGenVertexArrays(1, &leafVAO);
    GLuint quadVBO, quadEBO;
    glGenBuffers(1, &quadVBO);
    glGenBuffers(1, &quadEBO);
    glGenBuffers(1, &leafInstanceVBO);
    
    glBindVertexArray(leafVAO);
    
    // Quad 頂點
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Instance 數據（位置、顏色、大小、旋轉）
    std::vector<float> instanceData;
    for (const auto& leaf : leafParticles) {
        instanceData.push_back(leaf.position.x);
        instanceData.push_back(leaf.position.y);
        instanceData.push_back(leaf.position.z);
        instanceData.push_back(leaf.color.r);
        instanceData.push_back(leaf.color.g);
        instanceData.push_back(leaf.color.b);
        instanceData.push_back(leaf.size);
        instanceData.push_back(leaf.rotation);  // 添加旋轉值
    }
    
    std::cout << "[Tree] Uploading " << leafParticles.size() << " leaf instances, "
              << instanceData.size() << " floats" << std::endl;
    
    leafInstanceCount = (GLsizei)leafParticles.size();  // 保存葉子數量
    
    glBindBuffer(GL_ARRAY_BUFFER, leafInstanceVBO);
    glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(float), 
                 instanceData.data(), GL_STATIC_DRAW);
    
    // position (instanced)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);
    
    // color (instanced)
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);
    
    // size (instanced)
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);
    
    // rotation (instanced)
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(7 * sizeof(float)));
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);
    
    glBindVertexArray(0);
    
    // ===== Shaders =====
    branchShaderProgram = loadShader("../shaders/tree.vert", "../shaders/tree.frag");
    if (branchShaderProgram == 0) {
        std::cerr << "[Tree] Failed to load branch shaders!" << std::endl;
    }
    
    leafShaderProgram = loadShader("../shaders/tree_leaf.vert", "../shaders/tree_leaf.frag");
    if (leafShaderProgram == 0) {
        std::cerr << "[Tree] Failed to load leaf shaders!" << std::endl;
    } else {
        std::cout << "[Tree] Leaf shader loaded successfully, program ID: " << leafShaderProgram << std::endl;
    }
}

void Tree::setTransform(const glm::vec3& pos, float s, float ry) {
    position = pos;
    scale = s;
    rotY = ry;
}

void Tree::setLeafColorTheme(int theme) {
    leafColorTheme = theme;
    // 重新生成葉子顏色
    for (auto& leaf : leafParticles) {
        bool isDarkColor = randomFloat(0, 1) < 0.5f;
        float colorVar = randomFloat(0.9f, 1.0f);
        
        if (theme == 1) {
            // 橘色主題
            if (isDarkColor) {
                leaf.color = glm::vec3(0.85f * colorVar, 0.35f * colorVar, 0.08f * colorVar);
            } else {
                leaf.color = glm::vec3(0.98f * colorVar, 0.55f * colorVar, 0.15f * colorVar);
            }
        } else {
            // 紅色主題（默認）
            if (isDarkColor) {
                leaf.color = glm::vec3(0.58f * colorVar, 0.045f * colorVar, 0.075f * colorVar);
            } else {
                leaf.color = glm::vec3(0.93f * colorVar, 0.14f * colorVar, 0.045f * colorVar);
            }
        }
    }
    
    // 重新上傳葉子數據到GPU
    if (leafVAO && leafInstanceVBO && leafInstanceCount > 0) {
        std::vector<float> leafData;
        for (const auto& p : leafParticles) {
            leafData.push_back(p.position.x);
            leafData.push_back(p.position.y);
            leafData.push_back(p.position.z);
            leafData.push_back(p.color.r);
            leafData.push_back(p.color.g);
            leafData.push_back(p.color.b);
            leafData.push_back(p.size);
            leafData.push_back(p.rotation);
        }
        glBindBuffer(GL_ARRAY_BUFFER, leafInstanceVBO);
        glBufferData(GL_ARRAY_BUFFER, leafData.size() * sizeof(float), leafData.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void Tree::render(const glm::mat4& view, const glm::mat4& projection, 
                  const std::vector<Light>& lights, const Light& env_light, const glm::vec3& viewPos) {
    std::vector<Light> all = lights;
    all.push_back(env_light);
    
    glm::mat4 model(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, rotY, glm::vec3(0, 1, 0));
    model = glm::scale(model, glm::vec3(scale));
    
    // ===== 渲染樹幹/樹枝 =====
    glUseProgram(branchShaderProgram);
    
    glUniformMatrix4fv(glGetUniformLocation(branchShaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(branchShaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(branchShaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniform3fv(glGetUniformLocation(branchShaderProgram, "viewPos"), 1, &viewPos[0]);
    
    int n = (int)all.size();
    glUniform1i(glGetUniformLocation(branchShaderProgram, "numLights"), n);
    
    for (int i = 0; i < n && i < 100; ++i) {
        std::string base = "lights[" + std::to_string(i) + "]";
        glUniform3fv(glGetUniformLocation(branchShaderProgram, (base + ".position").c_str()), 1, glm::value_ptr(all[i].position));
        glUniform3fv(glGetUniformLocation(branchShaderProgram, (base + ".color").c_str()), 1, glm::value_ptr(all[i].color));
        glUniform1f(glGetUniformLocation(branchShaderProgram, (base + ".intensity").c_str()), all[i].intensity);
    }
    
    glBindVertexArray(branchVAO);
    glDrawElements(GL_TRIANGLES, branchIndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    
    // ===== 渲染葉子 Billboard =====
    glUseProgram(leafShaderProgram);
    
    glUniformMatrix4fv(glGetUniformLocation(leafShaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(leafShaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(leafShaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniform3fv(glGetUniformLocation(leafShaderProgram, "viewPos"), 1, &viewPos[0]);
    
    // 啟用混合以支持透明度
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // 保持深度測試和深度寫入開啟，讓樹葉能正確遮擋湖面
    // glDepthMask(GL_FALSE);  // 移除此行，讓樹葉寫入深度
    
    glBindVertexArray(leafVAO);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, leafInstanceCount);
    glBindVertexArray(0);
    
    // glDepthMask(GL_TRUE);  // 不需要了
    glDisable(GL_BLEND);
}
