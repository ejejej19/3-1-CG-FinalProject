#include "Lantern.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>

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
        std::cerr << "[Lantern] Shader compile error:\n" << log << "\n";
        glDeleteShader(s);
        return 0;
    }
    return s;
}

GLuint Lantern::loadShader(const char* vertexPath, const char* fragmentPath) {
    std::string vertexCode, fragmentCode;
    std::ifstream vShaderFile, fShaderFile;
    
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
        std::cerr << "ERROR::LANTERN::SHADER::FILE_NOT_READ: " << e.what() << std::endl;
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
        std::cerr << "ERROR::LANTERN::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        std::cerr << "ERROR::LANTERN::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "ERROR::LANTERN::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return program;
}

void Lantern::addBox(std::vector<float>& vertices, std::vector<unsigned int>& indices,
                     const glm::vec3& center, const glm::vec3& size, float isLight) {
    glm::vec3 min = center - size * 0.5f;
    glm::vec3 max = center + size * 0.5f;
    
    glm::vec3 corners[8] = {
        glm::vec3(min.x, min.y, min.z), glm::vec3(max.x, min.y, min.z),
        glm::vec3(max.x, max.y, min.z), glm::vec3(min.x, max.y, min.z),
        glm::vec3(min.x, min.y, max.z), glm::vec3(max.x, min.y, max.z),
        glm::vec3(max.x, max.y, max.z), glm::vec3(min.x, max.y, max.z)
    };
    
    struct Face {
        glm::vec3 normal;
        int indices[4];
    };
    
    Face faces[6] = {
        {glm::vec3(0, 0, -1), {0, 1, 2, 3}}, // front
        {glm::vec3(0, 0, 1),  {5, 4, 7, 6}}, // back
        {glm::vec3(-1, 0, 0), {4, 0, 3, 7}}, // left
        {glm::vec3(1, 0, 0),  {1, 5, 6, 2}}, // right
        {glm::vec3(0, 1, 0),  {3, 2, 6, 7}}, // top
        {glm::vec3(0, -1, 0), {4, 5, 1, 0}}  // bottom
    };
    
    for (int f = 0; f < 6; f++) {
        unsigned int faceBase = vertices.size() / 7;
        
        for (int i = 0; i < 4; i++) {
            glm::vec3 pos = corners[faces[f].indices[i]];
            vertices.push_back(pos.x);
            vertices.push_back(pos.y);
            vertices.push_back(pos.z);
            vertices.push_back(faces[f].normal.x);
            vertices.push_back(faces[f].normal.y);
            vertices.push_back(faces[f].normal.z);
            vertices.push_back(isLight);
        }
        
        indices.push_back(faceBase + 0);
        indices.push_back(faceBase + 1);
        indices.push_back(faceBase + 2);
        indices.push_back(faceBase + 0);
        indices.push_back(faceBase + 2);
        indices.push_back(faceBase + 3);
    }
}

void Lantern::initialize() {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    // 燈籠參數
    float poleHeight = 5.0f;
    float poleRadius = 0.15f;
    
    float lanternHeight = 1.4f;
    float lanternWidth = 1.0f;
    float frameThickness = 0.05f;
    
    // 1. 紅色柱子 (0 = 紅色)
    addBox(vertices, indices, glm::vec3(0, poleHeight * 0.5f, 0),
           glm::vec3(poleRadius * 2, poleHeight, poleRadius * 2), 0.0f);
    
    float lanternY = poleHeight + lanternHeight * 0.5f + 0.1f;
    
    // 2. 發光燈罩內部（暖黃色）
    float lightSize = lanternWidth * 0.88f;
    addBox(vertices, indices, glm::vec3(0, lanternY, 0),
           glm::vec3(lightSize, lanternHeight * 0.92f, lightSize), 1.0f);
    
    // 3. 外框四個黑色邊柱
    float cornerOffset = lanternWidth * 0.48f;
    float cornerSize = 0.06f;
    addBox(vertices, indices, glm::vec3(-cornerOffset, lanternY, -cornerOffset),
           glm::vec3(cornerSize, lanternHeight, cornerSize), 2.0f);
    addBox(vertices, indices, glm::vec3(cornerOffset, lanternY, -cornerOffset),
           glm::vec3(cornerSize, lanternHeight, cornerSize), 2.0f);
    addBox(vertices, indices, glm::vec3(-cornerOffset, lanternY, cornerOffset),
           glm::vec3(cornerSize, lanternHeight, cornerSize), 2.0f);
    addBox(vertices, indices, glm::vec3(cornerOffset, lanternY, cornerOffset),
           glm::vec3(cornerSize, lanternHeight, cornerSize), 2.0f);
    
    // 4. 頂部和底部橫框 (黑色)
    float frameY_top = lanternY + lanternHeight * 0.5f - frameThickness * 0.5f;
    float frameY_bot = lanternY - lanternHeight * 0.5f + frameThickness * 0.5f;
    addBox(vertices, indices, glm::vec3(0, frameY_top, 0),
           glm::vec3(lanternWidth, frameThickness, lanternWidth), 2.0f);
    addBox(vertices, indices, glm::vec3(0, frameY_bot, 0),
           glm::vec3(lanternWidth, frameThickness, lanternWidth), 2.0f);
    
    // 5. 十字網格 (黑色) - 每面一個十字
    float crossThickness = 0.04f;
    float crossDepth = 0.03f;
    float faceOffset = lanternWidth * 0.49f;  // 稍微突出於燈面
    
    // 水平橫條 (四面)
    addBox(vertices, indices, glm::vec3(0, lanternY, faceOffset),
           glm::vec3(lanternWidth * 0.9f, crossThickness, crossDepth), 2.0f);
    addBox(vertices, indices, glm::vec3(0, lanternY, -faceOffset),
           glm::vec3(lanternWidth * 0.9f, crossThickness, crossDepth), 2.0f);
    addBox(vertices, indices, glm::vec3(faceOffset, lanternY, 0),
           glm::vec3(crossDepth, crossThickness, lanternWidth * 0.9f), 2.0f);
    addBox(vertices, indices, glm::vec3(-faceOffset, lanternY, 0),
           glm::vec3(crossDepth, crossThickness, lanternWidth * 0.9f), 2.0f);
    
    // 垂直條 (四面)
    addBox(vertices, indices, glm::vec3(0, lanternY, faceOffset),
           glm::vec3(crossThickness, lanternHeight * 0.85f, crossDepth), 2.0f);
    addBox(vertices, indices, glm::vec3(0, lanternY, -faceOffset),
           glm::vec3(crossThickness, lanternHeight * 0.85f, crossDepth), 2.0f);
    addBox(vertices, indices, glm::vec3(faceOffset, lanternY, 0),
           glm::vec3(crossDepth, lanternHeight * 0.85f, crossThickness), 2.0f);
    addBox(vertices, indices, glm::vec3(-faceOffset, lanternY, 0),
           glm::vec3(crossDepth, lanternHeight * 0.85f, crossThickness), 2.0f);
    
    // 6. 屋頂 - 40層平滑斜面屋頂 (黑色)
    float baseY = poleHeight + lanternHeight + 0.12f;
    float roofTotalHeight = 0.7f;      // 屋頂總高度
    float roofBottomWidth = 1.6f;      // 底部寬度
    float roofTopWidth = 0.2f;         // 頂部寬度
    int roofLayers = 40;
    
    float layerHeight = roofTotalHeight / roofLayers;
    
    for (int i = 0; i < roofLayers; i++) {
        float t = (float)i / (roofLayers - 1);  // 0 到 1
        float layerWidth = roofBottomWidth * (1.0f - t) + roofTopWidth * t;
        float layerY = baseY + i * layerHeight;
        
        addBox(vertices, indices, glm::vec3(0, layerY, 0),
               glm::vec3(layerWidth, layerHeight * 1.1f, layerWidth), 2.0f);  // 稍微重疊避免縫隙
    }
    
    // 頂部尖頂裝飾
    float topY = baseY + roofTotalHeight + 0.05f;
    addBox(vertices, indices, glm::vec3(0, topY, 0),
           glm::vec3(0.12f, 0.2f, 0.12f), 2.0f);
    
    // 上傳到GPU
    indexCount = indices.size();
    
    glGenVertexArrays(1, &lanternVAO);
    glGenBuffers(1, &lanternVBO);
    glGenBuffers(1, &lanternEBO);
    
    glBindVertexArray(lanternVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, lanternVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), 
                 vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lanternEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), 
                 indices.data(), GL_STATIC_DRAW);
    
    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), 
                         (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // isLight flag
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(float), 
                         (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
    
    shaderProgram = loadShader("../shaders/lantern.vert", "../shaders/lantern.frag");
    
    std::cout << "[Lantern] Initialized with " << indexCount << " indices" << std::endl;
}

void Lantern::setTransform(const glm::vec3& pos, float s, float ry) {
    position = pos;
    scale = s;
    rotY = ry;
}

Light Lantern::getLight() const {
    // 光源位置在燈籠中心
    glm::vec3 lightPos = position + glm::vec3(0, 6.5f * scale, 0);
    return {lightPos, lightColor, lightIntensity, FLT_MAX};
}

void Lantern::render(const glm::mat4& view, const glm::mat4& projection,
                     const std::vector<Light>& lights, const Light& env_light,
                     const glm::vec3& viewPos) {
    if (!shaderProgram || !lanternVAO) return;
    
    glUseProgram(shaderProgram);
    
    glm::mat4 model(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, rotY, glm::vec3(0, 1, 0));
    model = glm::scale(model, glm::vec3(scale));
    
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 
                      1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 
                      1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 
                      1, GL_FALSE, &projection[0][0]);
    glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 
                1, &viewPos[0]);
    
    // 傳遞燈籠自己的光色
    glUniform3fv(glGetUniformLocation(shaderProgram, "lanternColor"), 
                1, &lightColor[0]);
    
    // 環境光照
    std::vector<Light> all = lights;
    all.push_back(env_light);
    int n = (int)all.size();
    glUniform1i(glGetUniformLocation(shaderProgram, "numLights"), n);
    
    for (int i = 0; i < n && i < 100; ++i) {
        std::string base = "lights[" + std::to_string(i) + "]";
        glUniform3fv(glGetUniformLocation(shaderProgram, (base + ".position").c_str()), 
                    1, glm::value_ptr(all[i].position));
        glUniform3fv(glGetUniformLocation(shaderProgram, (base + ".color").c_str()), 
                    1, glm::value_ptr(all[i].color));
        glUniform1f(glGetUniformLocation(shaderProgram, (base + ".intensity").c_str()), 
                   all[i].intensity);
    }
    
    glBindVertexArray(lanternVAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
