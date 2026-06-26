#include "Mountain.h"
#include "stb_image.h"
#include <cmath>

float Mountain::getHeight(float x, float z) {
    // 目標：遠看也要有「山脈」輪廓，而不是只有單一尖峰

    auto gauss = [](float x, float z, float cx, float cz, float A, float sigma2) {
        float dx = x - cx;
        float dz = z - cz;
        return A * std::exp(-(dx*dx + dz*dz) / sigma2);
    };

    float height = 0.0f;

    // 主峰（中心）
    // sigma2 越大越寬厚，遠看輪廓更像山
    height += gauss(x, z, 0.0f, -80.0f, 55.0f, 1600.0f);

    // 左右副峰
    height += gauss(x, z, -50.0f, -80.0f, 42.0f, 500.0f);  // 左峰
    height += gauss(x, z,  30.0f, -80.0f, 40.0f, 300.0f);  // 右峰

    // 讓整體像一條山脈
    float ridgeZ = std::exp(-((z + 78.0f) * (z + 78.0f)) / 4200.0f);
    float ridgeX = std::exp(-(x * x) / 26000.0f);
    height += 18.0f * ridgeZ * ridgeX;

    // 小起伏
    height += 2.0f * std::sin(0.06f * x + 0.08f * z);
    height += 1.3f * std::sin(0.10f * x - 0.05f * z);

    // 整體抬高
    // const float heightScale = 2.2f;  // 1.6f~3.0f 可調
    // height *= heightScale;

    // 湖邊淡出：範圍拉大一點，避免突然被砍平
    // float lakeRadius = 47.0f;
    // float distFromCenter = std::sqrt(x * x + z * z);
    // float fadeWidth = 18.0f; // 原本 5 太硬，容易一刀切
    // if (distFromCenter < lakeRadius + fadeWidth) {
    //     float fade = (distFromCenter - lakeRadius) / fadeWidth;
    //     height *= glm::clamp(fade, 0.0f, 1.0f);
    // }

    float edgeStart = 70.0f;   // 從 |x|>70 開始下降
    float edgeEnd   = 100.0f;   // 到邊界 |x|=100 變成 0
    float ax = std::abs(x);
    float edge = 1.0f - glm::clamp((ax - edgeStart) / (edgeEnd - edgeStart), 0.0f, 1.0f);
    height *= edge;

    return height;
}

void Mountain::initialize() {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    // ===== 3D 網格山脈 =====
    // 山脈範圍（縮小後配合大天空盒顯得渺小）
    const float xMin = -120.0f;
    const float xMax = 120.0f;
    const float zMin = -150.0f;  // 遠端
    const float zMax = -50.0f;   // 近端
    
    // 網格解析度
    const int gridX = 120;  // x 方向分段數
    const int gridZ = 80;   // z 方向分段數
    const float cellSizeX = (xMax - xMin) / gridX;
    const float cellSizeZ = (zMax - zMin) / gridZ;
    
    // 生成頂點
    for (int iz = 0; iz <= gridZ; ++iz) {
        for (int ix = 0; ix <= gridX; ++ix) {
            float x = xMin + ix * cellSizeX;
            float z = zMin + iz * cellSizeZ;
            float y = getHeight(x, z);
            
            // 邊緣淡出（x 方向）
            float edgeX = 1.0f;
            float fadeStartX = 90.0f;
            float fadeEndX = 110.0f;
            float ax = std::abs(x);
            if (ax > fadeStartX) {
                edgeX = 1.0f - glm::clamp((ax - fadeStartX) / (fadeEndX - fadeStartX), 0.0f, 1.0f);
                y *= edgeX;
            }
            
            // 邊緣淡出（z 方向近端）
            float edgeZ = 1.0f;
            float fadeStartZ = -60.0f;
            float fadeEndZ = -50.0f;
            if (z > fadeStartZ) {
                edgeZ = 1.0f - glm::clamp((z - fadeStartZ) / (fadeEndZ - fadeStartZ), 0.0f, 1.0f);
                y *= edgeZ;
            }
            
            // 計算法線 (用有限差分)
            float eps = 2.0f;
            float hL = getHeight(x - eps, z);
            float hR = getHeight(x + eps, z);
            float hD = getHeight(x, z - eps);
            float hU = getHeight(x, z + eps);
            glm::vec3 normal = glm::normalize(glm::vec3(hL - hR, 2.0f * eps, hD - hU));
            
            // 計算 heightFactor (用於著色)
            float maxHeight = 60.0f;
            float heightFactor = glm::clamp(y / maxHeight, 0.0f, 1.0f);
            
            // 計算 UV 座標 (用於紋理)
            float u = (x - xMin) / (xMax - xMin) * 8.0f;  // 重複貼圖 8 次
            float v = (z - zMin) / (zMax - zMin) * 8.0f;
            
            // 頂點: position(3) + normal(3) + heightFactor(1) + uv(2)
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            vertices.push_back(normal.x);
            vertices.push_back(normal.y);
            vertices.push_back(normal.z);
            vertices.push_back(heightFactor);
            vertices.push_back(u);
            vertices.push_back(v);
        }
    }
    
    // 生成索引 (三角形)
    for (int iz = 0; iz < gridZ; ++iz) {
        for (int ix = 0; ix < gridX; ++ix) {
            unsigned int topLeft = iz * (gridX + 1) + ix;
            unsigned int topRight = topLeft + 1;
            unsigned int bottomLeft = (iz + 1) * (gridX + 1) + ix;
            unsigned int bottomRight = bottomLeft + 1;
            
            // 第一個三角形
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);
            
            // 第二個三角形
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
    
    indexCount = (int)indices.size();

    glGenVertexArrays(1, &mountainVAO);
    glGenBuffers(1, &mountainVBO);
    glGenBuffers(1, &mountainEBO);

    glBindVertexArray(mountainVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mountainVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mountainEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    // UV 座標 (location = 3)
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(7 * sizeof(float)));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);

    shaderProgram = loadShader("../shaders/mountain.vert", "../shaders/mountain.frag");
    
    // 載入紋理
    rockTexture = loadTexture("../../static/ground_textures/ground_stone.png");
    snowTexture = loadTexture("../../static/ground_textures/ground_sand.jpg");  // 用沙地紋理模擬雪
}

void Mountain::render(const glm::mat4& view, const glm::mat4& projection,
                      std::vector<Light> lights, Light env_light) {
    lights.push_back(env_light);
    glUseProgram(shaderProgram);

    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform1f(glGetUniformLocation(shaderProgram, "snowLine"), 70.0f);
    glUniform1f(glGetUniformLocation(shaderProgram, "snowBlend"), 12.0f);
    glUniform1f(glGetUniformLocation(shaderProgram, "noiseScale"), 0.05f);
    glUniform1f(glGetUniformLocation(shaderProgram, "noiseAmp"), 10.0f);
    glUniform1f(glGetUniformLocation(shaderProgram, "slopeStart"), 0.55f);
    glUniform1f(glGetUniformLocation(shaderProgram, "slopeEnd"), 0.85f);

    int numLights = lights.size();
    glUniform1i(glGetUniformLocation(shaderProgram, "numLights"), numLights);
    for (int i = 0; i < numLights && i < 100; ++i) {
        std::string base = "lights[" + std::to_string(i) + "]";
        glUniform3fv(glGetUniformLocation(shaderProgram, (base + ".position").c_str()), 1, glm::value_ptr(lights[i].position));
        glUniform3fv(glGetUniformLocation(shaderProgram, (base + ".color").c_str()), 1, glm::value_ptr(lights[i].color));
        glUniform1f(glGetUniformLocation(shaderProgram, (base + ".intensity").c_str()), lights[i].intensity);
    }
    glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(Camera::position));
    
    // 綁定紋理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, rockTexture);
    glUniform1i(glGetUniformLocation(shaderProgram, "rockTexture"), 0);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, snowTexture);
    glUniform1i(glGetUniformLocation(shaderProgram, "snowTexture"), 1);
    
    glBindVertexArray(mountainVAO);

    glDisable(GL_CULL_FACE);
    // 保持深度測試，確保正確的前後關係
    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

GLuint Mountain::loadShader(const char* vertexPath, const char* fragmentPath) {
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

GLuint Mountain::loadTexture(const char* path) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    
    int width, height, nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = GL_RGB;
        if (nrChannels != 4) format = GL_RGB;
        
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        stbi_image_free(data);
        // std::cout << "Mountain texture loaded: " << path << " (" << width << "x" << height << ")" << std::endl;
    } else {
        std::cerr << "Failed to load mountain texture: " << path << std::endl;
        stbi_image_free(data);
    }
    
    return textureID;
}
