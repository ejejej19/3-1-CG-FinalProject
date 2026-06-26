#include "Lake.h"
#include <filesystem>

static GLuint create1x1TextureRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    unsigned char data[4] = { r, g, b, a };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

void Lake::initialize() {
    const int segments = 128;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    // ---- 生成不規則形狀湖面 ----
    // 中心點
    vertices.push_back(0.0f);
    vertices.push_back(waterLevel);
    vertices.push_back(0.0f);
    vertices.push_back(0.5f);
    vertices.push_back(0.5f);
    vertices.push_back(0.0f);
    vertices.push_back(1.0f);
    vertices.push_back(0.0f);
    
    // 用噪聲函數產生不規則邊緣
    auto noise = [](float angle) {
        // 多個正弦波疊加產生自然的不規則感
        float n = 1.0f;
        n += 0.15f * std::sin(angle * 2.0f + 0.5f);   // 大
        n += 0.10f * std::sin(angle * 3.0f + 1.2f);   // 中
        n += 0.08f * std::sin(angle * 5.0f + 2.7f);   // 小
        n += 0.05f * std::sin(angle * 7.0f + 0.3f);   // 細節
        n += 0.03f * std::sin(angle * 11.0f + 1.8f);  // 更多細節
        return n;
    };
    
    // 圓周上的點（帶不規則半徑）
    for (int i = 0; i <= segments; ++i) {
        float angle = (float)i / segments * 2.0f * 3.14159265f;
        float radiusMod = noise(angle);
        float r = lakeRadius * radiusMod;
        float x = std::cos(angle) * r;
        float z = std::sin(angle) * r;
        
        vertices.push_back(x);
        vertices.push_back(waterLevel);
        vertices.push_back(z);
        vertices.push_back((std::cos(angle) + 1.0f) * 0.5f * 10.0f);
        vertices.push_back((std::sin(angle) + 1.0f) * 0.5f * 10.0f);
        vertices.push_back(0.0f);
        vertices.push_back(1.0f);
        vertices.push_back(0.0f);
    }
    
    // 建立三角形索引 (扇形)
    for (int i = 1; i <= segments; ++i) {
        indices.push_back(0);
        indices.push_back(i);
        indices.push_back(i + 1);
    }
    
    indexCount = static_cast<int>(indices.size());

    glGenVertexArrays(1, &lakeVAO);
    glGenBuffers(1, &lakeVBO);
    glGenBuffers(1, &lakeEBO);

    glBindVertexArray(lakeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lakeVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lakeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // uv
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // normal
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    shaderProgram = loadShader("../shaders/water.vert", "../shaders/water.frag");
    
    // waterTexture： 1x1，用 uniform waterColor 控制水色
    waterTex = create1x1TextureRGBA(21, 63, 91, 255);
    waterNormalTex = create1x1TextureRGBA(128, 128, 255, 255);
}

void Lake::render(const glm::mat4& view, const glm::mat4& projection,
                  const glm::mat4& reflViewProj,
                  std::vector<Light> lights, Light env_light, float time, GLuint reflectionTex) {
    lights.push_back(env_light);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDepthMask(GL_FALSE); // 半透明水面

    glUseProgram(shaderProgram);

    // ---- 綁定水的貼圖到固定 unit ----
    // water.frag: uniform sampler2D waterTexture / normalTexture
    glUniform1i(glGetUniformLocation(shaderProgram, "waterTexture"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "normalTexture"), 1);
    glUniform1i(glGetUniformLocation(shaderProgram, "reflectionTexture"), 2);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, waterTex);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, waterNormalTex);
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, reflectionTex);
    // ---- matrices / time ----
    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "reflViewProj"), 1, GL_FALSE, glm::value_ptr(reflViewProj));
    glUniform1f(glGetUniformLocation(shaderProgram, "time"), time);

    // ---- lights ----
    int numLights = static_cast<int>(lights.size());
    glUniform1i(glGetUniformLocation(shaderProgram, "numLights"), numLights);

    for (int i = 0; i < numLights && i < 100; ++i) {
        std::string base = "lights[" + std::to_string(i) + "]";
        glUniform3fv(glGetUniformLocation(shaderProgram, (base + ".position").c_str()), 1, glm::value_ptr(lights[i].position));
        glUniform3fv(glGetUniformLocation(shaderProgram, (base + ".color").c_str()), 1, glm::value_ptr(lights[i].color));
        glUniform1f(glGetUniformLocation(shaderProgram, (base + ".intensity").c_str()), lights[i].intensity);
    }

    // ---- water params ----
    glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(Camera::position));
    glUniform3f(glGetUniformLocation(shaderProgram, "waterColor"), 0.06f, 0.09f, 0.14f);  // 更淡的藍色，更透明
    glUniform1f(glGetUniformLocation(shaderProgram, "waterAlpha"), 0.45f);  // 基礎透明度
    glUniform1f(glGetUniformLocation(shaderProgram, "lakeRadius"), lakeRadius);

    // ---- draw ----
    glBindVertexArray(lakeVAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

GLuint Lake::loadShader(const char* vertexPath, const char* fragmentPath) {
    // std::cout << "CWD = " << std::filesystem::current_path() << "\n";
    // std::cout << "vertexPath = " << vertexPath << "\n";
    // std::cout << "fragmentPath = " << fragmentPath << "\n";

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
    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    GLint success = 0;
    GLchar infoLog[512];

    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
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
