#include "Torii.h"
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
        std::cerr << "[Torii] Shader compile error:\n" << log << "\n";
        glDeleteShader(s);
        return 0;
    }
    return s;
}

GLuint Torii::loadShader(const char* vertexPath, const char* fragmentPath) {
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
        std::cerr << "ERROR::TORII::SHADER::FILE_NOT_READ: " << e.what() << std::endl;
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
        std::cerr << "ERROR::TORII::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        std::cerr << "ERROR::TORII::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "ERROR::TORII::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return program;
}

void Torii::addBox(std::vector<float>& vertices, std::vector<unsigned int>& indices,
                   const glm::vec3& center, const glm::vec3& size, float partType) {
    glm::vec3 min = center - size * 0.5f;
    glm::vec3 max = center + size * 0.5f;
    
    unsigned int baseIdx = vertices.size() / 6;
    
    // 8個頂點的位置
    glm::vec3 corners[8] = {
        glm::vec3(min.x, min.y, min.z), glm::vec3(max.x, min.y, min.z),
        glm::vec3(max.x, max.y, min.z), glm::vec3(min.x, max.y, min.z),
        glm::vec3(min.x, min.y, max.z), glm::vec3(max.x, min.y, max.z),
        glm::vec3(max.x, max.y, max.z), glm::vec3(min.x, max.y, max.z)
    };
    
    // 6個面的法線和頂點索引
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
            vertices.push_back(partType);
        }
        
        // 兩個三角形
        indices.push_back(faceBase + 0);
        indices.push_back(faceBase + 1);
        indices.push_back(faceBase + 2);
        indices.push_back(faceBase + 0);
        indices.push_back(faceBase + 2);
        indices.push_back(faceBase + 3);
    }
}

void Torii::addCurvedBeam(std::vector<float>& vertices, std::vector<unsigned int>& indices,
                          const glm::vec3& start, const glm::vec3& end, 
                          float width, float height, float curve, float partType) {
    const int segments = 16;
    float length = glm::length(end - start);
    glm::vec3 dir = glm::normalize(end - start);
    glm::vec3 up = glm::vec3(0, 1, 0);
    glm::vec3 right = glm::normalize(glm::cross(dir, up));
    
    for (int i = 0; i <= segments; i++) {
        float t = (float)i / segments;
        float curveOffset = curve * sin(t * 3.14159f); // 中間最高
        
        glm::vec3 pos = start + dir * (length * t) + up * curveOffset;
        
        // 4個角的頂點
        glm::vec3 corners[4] = {
            pos + right * width * 0.5f - up * height * 0.5f,
            pos - right * width * 0.5f - up * height * 0.5f,
            pos - right * width * 0.5f + up * height * 0.5f,
            pos + right * width * 0.5f + up * height * 0.5f
        };
        
        unsigned int baseIdx = vertices.size() / 7;
        
        for (int j = 0; j < 4; j++) {
            vertices.push_back(corners[j].x);
            vertices.push_back(corners[j].y);
            vertices.push_back(corners[j].z);
            
            // 簡化法線計算
            glm::vec3 normal = (j < 2) ? -up : up;
            if (j == 0 || j == 3) normal += right;
            else normal -= right;
            normal = glm::normalize(normal);
            
            vertices.push_back(normal.x);
            vertices.push_back(normal.y);
            vertices.push_back(normal.z);
            vertices.push_back(partType);
        }
        
        if (i > 0) {
            unsigned int prevBase = baseIdx - 4;
            // 連接前一個截面
            for (int j = 0; j < 4; j++) {
                int next = (j + 1) % 4;
                indices.push_back(prevBase + j);
                indices.push_back(baseIdx + j);
                indices.push_back(baseIdx + next);
                indices.push_back(prevBase + j);
                indices.push_back(baseIdx + next);
                indices.push_back(prevBase + next);
            }
        }
    }
}

void Torii::initialize() {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    // 鳥居參數 (明神鳥居樣式)
    float pillarHeight = 12.0f;
    float pillarRadius = 0.6f;
    float pillarSpacing = 8.0f;
    
    // 笠木 (上層橫樑，兩端上翹)
    float kasagiWidth = 11.0f;        // 比柱子寬
    float kasagiHeight = 0.9f;
    float kasagiThickness = 0.7f;
    float kasagiCurve = -0.4f;         // 增加弧度
    
    // 島木 (第二層橫樑)
    float shimakiWidth = 10.0f;
    float shimakiHeight = 0.9f;
    float shimakiThickness = 0.6f;
    float shimakiCurve = -0.4f;
    float shimakiY = pillarHeight-0.3f;  // 稍微低於笠木
    
    // 貫 (中橫樑)
    float nukiHeight = 8.0f;
    float nukiThickness = 0.5f;
    
    // 柱頂楔形裝飾
    float kusabiHeight = 0.4f;
    float kusabiWidth = 0.3f;
    
    // 左柱 (稍微向內傾斜)
    glm::vec3 leftPillarPos(-pillarSpacing * 0.5f, pillarHeight * 0.5f, 0);
    addBox(vertices, indices, leftPillarPos, 
           glm::vec3(pillarRadius * 2, pillarHeight, pillarRadius * 2));
    
    // 左柱頂楔形裝飾
    glm::vec3 leftKusabi(-pillarSpacing * 0.5f, pillarHeight + kusabiHeight * 0.5f, 0);
    addBox(vertices, indices, leftKusabi,
           glm::vec3(kusabiWidth, kusabiHeight, pillarRadius * 2));
    
    // 右柱
    glm::vec3 rightPillarPos(pillarSpacing * 0.5f, pillarHeight * 0.5f, 0);
    addBox(vertices, indices, rightPillarPos, 
           glm::vec3(pillarRadius * 2, pillarHeight, pillarRadius * 2));
    
    // 右柱頂楔形裝飾
    glm::vec3 rightKusabi(pillarSpacing * 0.5f, pillarHeight + kusabiHeight * 0.5f, 0);
    addBox(vertices, indices, rightKusabi,
           glm::vec3(kusabiWidth, kusabiHeight, pillarRadius * 2));
    
    // 笠木 (上層橫樑) - 兩端上翹的弧度 - 黑色
    glm::vec3 kasagiStart(-kasagiWidth * 0.5f, pillarHeight + 0.5f, 0);
    glm::vec3 kasagiEnd(kasagiWidth * 0.5f, pillarHeight + 0.5f, 0);
    addCurvedBeam(vertices, indices, kasagiStart, kasagiEnd, 
                  kasagiThickness, kasagiHeight, kasagiCurve, 1.0f);
    
    // 島木 (第二層橫樑) - 輕微弧度
    glm::vec3 shimakiStart(-shimakiWidth * 0.5f, shimakiY, 0);
    glm::vec3 shimakiEnd(shimakiWidth * 0.5f, shimakiY, 0);
    addCurvedBeam(vertices, indices, shimakiStart, shimakiEnd,
                  shimakiThickness, shimakiHeight, shimakiCurve);
    
    // 貫 (中橫樑) - 直的
    glm::vec3 nukiStart(-pillarSpacing * 0.5f, nukiHeight, 0);
    glm::vec3 nukiEnd(pillarSpacing * 0.5f, nukiHeight, 0);
    addCurvedBeam(vertices, indices, nukiStart, nukiEnd, 
                  nukiThickness, nukiThickness, 0.0f);
    
    // 額束 (中間垂直連接柱) - 連接島木和貫
    float gakuzukaHeight = shimakiY - nukiHeight;
    float gakuzukaWidth = 0.5f;
    float gakuzukaThickness = 0.5f;
    glm::vec3 gakuzukaPos(0, nukiHeight + gakuzukaHeight * 0.5f, 0);
    addBox(vertices, indices, gakuzukaPos,
           glm::vec3(gakuzukaWidth, gakuzukaHeight, gakuzukaThickness));
    
    // 上傳到GPU
    indexCount = indices.size();
    
    glGenVertexArrays(1, &toriiVAO);
    glGenBuffers(1, &toriiVBO);
    glGenBuffers(1, &toriiEBO);
    
    glBindVertexArray(toriiVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, toriiVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), 
                 vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, toriiEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), 
                 indices.data(), GL_STATIC_DRAW);
    
    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), 
                         (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // partType
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(float), 
                         (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
    
    // 載入shader
    shaderProgram = loadShader("../shaders/torii.vert", "../shaders/torii.frag");
    
    std::cout << "[Torii] Initialized with " << indexCount << " indices" << std::endl;
}

void Torii::setTransform(const glm::vec3& pos, float s, float ry) {
    position = pos;
    scale = s;
    rotY = ry;
}

void Torii::render(const glm::mat4& view, const glm::mat4& projection,
                   const std::vector<Light>& lights, const Light& env_light,
                   const glm::vec3& viewPos) {
    if (!shaderProgram || !toriiVAO) return;
    
    glUseProgram(shaderProgram);
    
    // 模型矩陣
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
    
    // 光照
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
    
    glBindVertexArray(toriiVAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
