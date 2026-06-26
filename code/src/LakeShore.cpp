#include "LakeShore.h"

void LakeShore::initialize(const std::string& texturePath) {
    // 建立環形湖岸 (湖面外圍的陸地)
    // 內圈是湖邊（不規則形狀），外圈延伸到場景邊緣
    const int segments = 128;  // 與湖面相同
    const float baseInnerRadius = 80.0f;   // 配合放大的湖面
    const float outerRadius = 500.0f;  // 擴大陸地配合大天空盒
    const float groundY = -0.15f;      // 地面略低於水面，避免 Z-fighting
    
    // 與湖面相同的噪聲函數產生配合的不規則邊緣
    auto noise = [](float angle) {
        float n = 1.0f;
        n += 0.15f * std::sin(angle * 2.0f + 0.5f);
        n += 0.10f * std::sin(angle * 3.0f + 1.2f);
        n += 0.08f * std::sin(angle * 5.0f + 2.7f);
        n += 0.05f * std::sin(angle * 7.0f + 0.3f);
        n += 0.03f * std::sin(angle * 11.0f + 1.8f);
        return n;
    };
    
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    // 建立環形頂點
    for (int i = 0; i <= segments; ++i) {
        float angle = (float)i / segments * 2.0f * 3.14159265f;
        float cosA = std::cos(angle);
        float sinA = std::sin(angle);
        
        // 內圈半徑（不規則，與湖面配合）
        float innerRadius = baseInnerRadius * noise(angle);
        
        // 內圈頂點 (湖邊)
        float xInner = cosA * innerRadius;
        float zInner = sinA * innerRadius;
        vertices.push_back(xInner);
        vertices.push_back(groundY);
        vertices.push_back(zInner);
        vertices.push_back((cosA + 1.0f) * 0.5f * 20.0f);   // u
        vertices.push_back((sinA + 1.0f) * 0.5f * 20.0f);   // v
        vertices.push_back(0.0f);   // normal x
        vertices.push_back(1.0f);   // normal y
        vertices.push_back(0.0f);   // normal z
        
        // 外圈頂點
        float xOuter = cosA * outerRadius;
        float zOuter = sinA * outerRadius;
        vertices.push_back(xOuter);
        vertices.push_back(groundY);
        vertices.push_back(zOuter);
        vertices.push_back((cosA + 1.0f) * 0.5f * 40.0f);   // u
        vertices.push_back((sinA + 1.0f) * 0.5f * 40.0f);   // v
        vertices.push_back(0.0f);   // normal x
        vertices.push_back(1.0f);   // normal y
        vertices.push_back(0.0f);   // normal z
    }
    
    // 建立三角形索引 (環形)
    for (int i = 0; i < segments; ++i) {
        int innerCurrent = i * 2;
        int outerCurrent = i * 2 + 1;
        int innerNext = (i + 1) * 2;
        int outerNext = (i + 1) * 2 + 1;
        
        // 第一個三角形
        indices.push_back(innerCurrent);
        indices.push_back(outerCurrent);
        indices.push_back(innerNext);
        
        // 第二個三角形
        indices.push_back(innerNext);
        indices.push_back(outerCurrent);
        indices.push_back(outerNext);
    }
    
    indexCount = indices.size();

    glGenVertexArrays(1, &shoreVAO);
    glGenBuffers(1, &shoreVBO);
    glGenBuffers(1, &shoreEBO);

    glBindVertexArray(shoreVAO);

    glBindBuffer(GL_ARRAY_BUFFER, shoreVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shoreEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // 位置
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // 紋理座標
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // 法向量
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    shoreTexture = loadTexture(texturePath);
    shaderProgram = loadShader("../shaders/ground.vert", "../shaders/ground.frag");
}

void LakeShore::render(const glm::mat4& view, const glm::mat4& projection, 
                       std::vector<Light> lights, Light env_light) {
    lights.push_back(env_light);

    glUseProgram(shaderProgram);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, shoreTexture);
    glUniform1i(glGetUniformLocation(shaderProgram, "groundTexture"), 0); // 

    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // 傳遞光源資訊
    int numLights = lights.size();
    glUniform1i(glGetUniformLocation(shaderProgram, "numLights"), numLights);
    for (int i = 0; i < numLights && i < 100; ++i) {
        std::string base = "lights[" + std::to_string(i) + "]";
        glUniform3fv(glGetUniformLocation(shaderProgram, (base + ".position").c_str()), 1, glm::value_ptr(lights[i].position));
        glUniform3fv(glGetUniformLocation(shaderProgram, (base + ".color").c_str()), 1, glm::value_ptr(lights[i].color));
        glUniform1f(glGetUniformLocation(shaderProgram, (base + ".intensity").c_str()), lights[i].intensity);
    }

    glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(Camera::position));

    glBindVertexArray(shoreVAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

GLuint LakeShore::loadTexture(const std::string& path) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    int width, height, nrChannels;
    unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        std::cerr << "Failed to load texture: " << path << std::endl;
        // 建立預設草地顏色紋理
        unsigned char defaultColor[] = {40, 80, 40, 255};
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, defaultColor);
    }

    stbi_image_free(data);
    return textureID;
}

GLuint LakeShore::loadShader(const char* vertexPath, const char* fragmentPath) {
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;
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
        std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
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
        return 0;
    }

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        return 0;
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return program;
}
