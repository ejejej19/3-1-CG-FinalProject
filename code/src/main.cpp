#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <SFML/Audio.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Sky.h"
#include "utils.h"
#include "Light.h"
#include "Camera.h"
#include "Skybox.h"
#include "Ground.h"
#include "Lake.h"
#include "LakeShore.h"
#include "Mountain.h"
#include "Tree.h"
#include "Torii.h"
#include "Lantern.h"
#include "House.h"
#include "ObjLoader.h"
#include "Particle.h"
#include "Fireworks.h"
#include "ParticleRenderer.h"

// 處理按鍵输入
bool enterKeyPressed = false;
bool switchKeyPressed = false;
void processInput(GLFWwindow *window, float deltaTime, Fireworks & firework)
{
    // 降低移動速度營造大場景感（天空很大，人很小）
    float speed = 15.0f;
    // 按住 Shift 鍵加速 (4倍速)
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        speed *= 4.0f;
    }
    const float cameraSpeed = speed * deltaTime;

    // camera 移動 (WASD)
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
        Camera::move(Camera::front * cameraSpeed); // 向前
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
        Camera::move(-Camera::front * cameraSpeed); // 向后
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
        Camera::move(-glm::normalize(glm::cross(Camera::front, Camera::up)) * cameraSpeed); // 向左
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
        Camera::move(glm::normalize(glm::cross(Camera::front, Camera::up)) * cameraSpeed); // 向右
    }

    // 視角調整（方向鍵)
    const float rotationSpeed = 100.0f * deltaTime; // 旋轉速度 (加快)
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS){
        Camera::adjustPitch(rotationSpeed); // 向上看
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS){
        Camera::adjustPitch(-rotationSpeed); // 向下看
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS){
        Camera::adjustYaw(-rotationSpeed); // 向左看
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS){
        Camera::adjustYaw(rotationSpeed); // 向右看
    }

    // 切换烟花
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
    {
        if (!switchKeyPressed)
        {
            switchKeyPressed = true; // 标记为已按下
            firework.switchFirework();
        }
    }
    else
    {
        // 当按键松开时重置标记
        switchKeyPressed = false;
    }

    // 发射烟花
    if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS)
    {
        if (!enterKeyPressed)
        {
            // 仅在按下时触发一次
            enterKeyPressed = true; // 标记为已按下
            launchSound[launch_index].play(); // 发射音效
            launch_index = (launch_index + 1) % MAX_SOUNDS;
            firework.launch();
        }
    }
    else
    {
        // 当按键松开时重置标记
        enterKeyPressed = false;
    }
}

// 湖邊場景光源 - 煙花爆炸時會動態添加光源
std::vector<Light> lights = {};
// 月光 - 稍微偏藍色調營造夜晚氛圍
Light env_light = {{0.0f, 500.0f, 0.0f}, {0.12f, 0.14f, 0.18f}, 600.0f, FLT_MAX};

// 反射
GLuint reflFBO, reflTex, reflDepth;
int reflW=1024, reflH=683;

// HDR + Bloom
GLuint hdrFBO, hdrColorBuffer, hdrDepthBuffer;
GLuint pingpongFBO[2], pingpongBuffer[2];
GLuint blurShader, brightExtractShader, tonemapShader;
GLuint quadVAO = 0, quadVBO;

// 創建全屏四邊形
static void initQuad() {
    if (quadVAO != 0) return;
    
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);
}

static void renderQuad() {
    if (quadVAO == 0) initQuad();
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

static GLuint loadPostProcessShader(const char* vertPath, const char* fragPath) {
    std::string vertexCode, fragmentCode;
    std::ifstream vShaderFile, fShaderFile;
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    
    try {
        vShaderFile.open(vertPath);
        fShaderFile.open(fragPath);
        std::stringstream vShaderStream, fShaderStream;
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        vShaderFile.close();
        fShaderFile.close();
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    }
    catch (std::ifstream::failure& e) {
        std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
    }
    
    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();
    
    GLuint vertex, fragment;
    int success;
    char infoLog[512];
    
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

static void initHDR(int width, int height) {
    // HDR Framebuffer
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    
    glGenTextures(1, &hdrColorBuffer);
    glBindTexture(GL_TEXTURE_2D, hdrColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, hdrColorBuffer, 0);
    
    glGenRenderbuffers(1, &hdrDepthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, hdrDepthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, hdrDepthBuffer);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "HDR Framebuffer not complete!" << std::endl;
    
    // Ping-pong framebuffers for blur (降低解析度提升性能)
    int bloomWidth = width / 2;   // Bloom 用一半解析度
    int bloomHeight = height / 2;
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongBuffer);
    for (unsigned int i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongBuffer[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, bloomWidth, bloomHeight, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongBuffer[i], 0);
        
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cerr << "Ping-pong Framebuffer " << i << " not complete!" << std::endl;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Load shaders
    blurShader = loadPostProcessShader("../shaders/quad.vert", "../shaders/blur.frag");
    brightExtractShader = loadPostProcessShader("../shaders/quad.vert", "../shaders/bright_extract.frag");
    tonemapShader = loadPostProcessShader("../shaders/quad.vert", "../shaders/tonemap.frag");
    
    initQuad();
}

static void initReflectionFBO() {
    glGenFramebuffers(1, &reflFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, reflFBO);

    glGenTextures(1, &reflTex);
    glBindTexture(GL_TEXTURE_2D, reflTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, reflW, reflH, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, reflTex, 0);

    glGenRenderbuffers(1, &reflDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, reflDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, reflW, reflH);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, reflDepth);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "Reflection FBO not complete!\n";

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

int main()
{
    // 初始化 GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // 指定OpenGL版本为3.3，核心模式
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 创建窗口
    int window_width = 1800, window_height = 1200;
    GLFWwindow *window = glfwCreateWindow(window_width, window_height, "Firework", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    // 设置当前窗口为当前上下文，指定窗口大小变化时的回调函数，设置鼠标回调
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // 初始化 GLEW
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // 开启深度测试
    glEnable(GL_DEPTH_TEST);

    // 設定一個基準狀態，避免被 reflection pass 汙染
    // glDepthFunc(GL_LESS);
    // glDepthMask(GL_TRUE);

    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_BACK);
    // glFrontFace(GL_CCW);

    // glDisable(GL_BLEND);
    // glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    // 初始化反射
    initReflectionFBO();
    
    // 初始化 HDR + Bloom
    initHDR(window_width, window_height);

    // 初始化音效
    for (int i = 0; i < MAX_SOUNDS; ++i)
    {
        if (!launchBuffer[i].loadFromFile("../../static/audio/launch.wav"))
        {
            std::cerr << "Failed to load launch.wav" << std::endl;
            return -1;
        }
        launchSound[i].setBuffer(launchBuffer[i]);
        if (!explosionBuffer[i].loadFromFile("../../static/audio/explosion.wav"))
        {
            std::cerr << "Failed to load explosion sound!" << std::endl;
            return -1;
        }
        explosionSound[i].setBuffer(explosionBuffer[i]);
    }

    // 加載並初始化湖邊場景
    Skybox skybox;
    skybox.initialize();

    // 湖岸 (環形陸地)
    LakeShore lakeshore;
    lakeshore.initialize("../../static/ground_textures/ground_grass.png");

    // 湖面
    Lake lake;
    lake.initialize();

    // 雪山背景
    Mountain mountain;
    mountain.initialize();

    // 樹背景 - 分散在湖岸邊，高低錯落
    Tree tree1;
    tree1.initialize();
    tree1.setTransform(glm::vec3(-5.0f, 0.0f, -20.0f), 1.2f, glm::radians(25.0f));  // 最矮
    Tree tree2;
    tree2.initialize();
    tree2.setTransform(glm::vec3(-7.0f, 0.0f,-47.0f), 1.8f, glm::radians(25.0f));  // 最高
    Tree tree3;
    tree3.initialize();
    tree3.setTransform(glm::vec3(-35.0f, 0.0f, -40.0f), 1.0f, glm::radians(65.0f));  // 小
    Tree tree4;
    tree4.initialize();
    tree4.setTransform(glm::vec3(-37.0f, 0.0f, -20.0f), 1.5f, glm::radians(-130.0f));  // 中
    Tree tree5;
    tree5.initialize();
    tree5.setTransform(glm::vec3(-3.0f, 0.0f, -30.0f), 1.3f, glm::radians(130.0f));  // 橘色樹1
    tree5.setLeafColorTheme(1);  // 設置為橘色
    Tree tree6;
    tree6.initialize();
    tree6.setTransform(glm::vec3(-50.0f, 0.0f, -45.0f), 1.4f, glm::radians(20.0f));  // 橘色樹2
    tree6.setLeafColorTheme(1);  // 設置為橘色

    // 鳥居 - 放在湖面上
    Torii torii1;
    torii1.initialize();
    torii1.setTransform(glm::vec3(30.0f, 0.0f, -8.0f), 1.0f, 0.0f);

    // 日式燈籠 - 沿着湖岸分布
    std::vector<Lantern> lanterns;
    lanterns.resize(8);
    
    // 初始化所有燈籠
    for (auto& lantern : lanterns) {
        lantern.initialize();
    }
    
    // 設置燈籠位置（環繞湖岸）
    lanterns[0].setTransform(glm::vec3(-65.0f, 0.0f, -30.0f), 1.0f, 0.0f);
    lanterns[1].setTransform(glm::vec3(-65.0f, 0.0f, 0.0f), 1.0f, 0.0f);
    lanterns[2].setTransform(glm::vec3(-65.0f, 0.0f, 30.0f), 1.0f, 0.0f);
    lanterns[3].setTransform(glm::vec3(-30.0f, 0.0f, 65.0f), 1.0f, 0.0f);
    lanterns[4].setTransform(glm::vec3(0.0f, 0.0f, 65.0f), 1.0f, 0.0f);
    lanterns[5].setTransform(glm::vec3(30.0f, 0.0f, 65.0f), 1.0f, 0.0f);
    lanterns[6].setTransform(glm::vec3(65.0f, 0.0f, 30.0f), 1.0f, 0.0f);
    lanterns[7].setTransform(glm::vec3(65.0f, 0.0f, -15.0f), 1.0f, 0.0f);

    // 日式村落房子
    std::vector<House> houses;
    houses.resize(1);
    
    for (auto& house : houses) {
        house.initialize();
    }
    
    // 設置神社位置（在鳥居和山之間）
    houses[0].setPosition(glm::vec3(-30.0f, 0.0f, -40.0f));
    // 計算面向湖中心(0,0,0)的角度: atan2(30, 40) * 180/PI ≈ 36.87度
    // 正面朝+Z，需要旋轉讓+Z指向湖中心
    houses[0].setRotation(180.0f + 36.87f);  // 面向湖中心
    houses[0].setScale(1.0f);

    // 不再使用雲朵 Sky 物件
    // Sky sky;
    // bool hasSky = false;

    Fireworks firework;
    firework.initialize();
    float lastFrame = 0.0f; // 上一帧的时间

    // 主循环
    while (!glfwWindowShouldClose(window))
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        auto frameStart = Clock::now();

        // 1. 计算帧时间
        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // 2. 处理输入
        auto inputStart = Clock::now();
        processInput(window, deltaTime, firework);

        // 3. 清屏
        auto clearStart = Clock::now();

        // glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 4. 渲染固定背景
        auto renderStart = Clock::now();
        glm::mat4 view = Camera::getViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(60.0f), float(window_width) / window_height, 0.5f, 1000.0f); 
        
        // 5. 更新粒子
        auto particleUpdateStart = Clock::now();
        
        // 添加燈籠光源到lights向量（在煩火更新前）
        std::vector<Light> staticLights;
        for (const auto& lantern : lanterns) {
            staticLights.push_back(lantern.getLight());
        }
        
        firework.update(deltaTime, lights);
        
        // ===== Reflection pass：把「天空/山/煙火粒子」畫到 reflTex =====
        glBindFramebuffer(GL_FRAMEBUFFER, reflFBO);
        glViewport(0, 0, reflW, reflH);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 只在「渲染反射」時鏡射相機（不要影響 update/launch）
        glm::vec3 savedPos   = Camera::position;
        glm::vec3 savedFront = Camera::front;
        glm::vec3 savedUp    = Camera::up;

        const float waterY = 0.0f;
        Camera::position.y = 2.0f * waterY - savedPos.y;
        Camera::front.y    = -savedFront.y;
        Camera::up.y       = -savedUp.y;

        glm::mat4 reflView = Camera::getViewMatrix();

        // 反射渲染：先渲染不透明物體
        skybox.render(reflView, projection, 0.15f);
        mountain.render(reflView, projection, lights, env_light);
        torii1.render(reflView, projection, lights, env_light, Camera::position);
        
        // 渲染房子
        for (auto& house : houses) {
            house.render(reflView, projection, lights, env_light, Camera::position, currentFrame);
        }
        
        // 渲染燈籠
        for (auto& lantern : lanterns) {
            lantern.render(reflView, projection, lights, env_light, Camera::position);
        }
        
        // 最後渲染樹（包含半透明葉子）
        tree1.render(reflView, projection, lights, env_light, Camera::position);
        tree2.render(reflView, projection, lights, env_light, Camera::position);
        tree3.render(reflView, projection, lights, env_light, Camera::position);
        tree4.render(reflView, projection, lights, env_light, Camera::position);
        tree5.render(reflView, projection, lights, env_light, Camera::position);
        tree6.render(reflView, projection, lights, env_light, Camera::position);

        // 反射粒子（要開 blending，且通常不寫 depth）
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);
        firework.render(reflView, projection);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        // 計算反射相機的 view-projection 矩陣（在還原相機前）
        glm::mat4 reflViewProj = projection * reflView;

        // 還原相機
        Camera::position = savedPos;
        Camera::front    = savedFront;
        Camera::up       = savedUp;

        // ===== HDR 主場景渲染 =====
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glViewport(0, 0, window_width, window_height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // 渲染場景到 HDR buffer
        // 不透明物體先渲染（寫入深度緩衝區）
        skybox.render(view, projection, 0.45f);
        lakeshore.render(view, projection, lights, env_light);
        
        // 渲染雪山（不透明）
        auto mountainRenderStart = Clock::now();
        mountain.render(view, projection, lights, env_light);
        
        // 渲染神社和鳥居（不透明，在樹之前）
        torii1.render(view, projection, lights, env_light, Camera::position);
        
        // 渲染房子（不透明，在樹之前）
        for (auto& house : houses) {
            house.render(view, projection, lights, env_light, Camera::position, currentFrame);
        }
        
        // 渲染燈籠（包含燈籠光源，不透明）
        std::vector<Light> allLights = staticLights;
        allLights.insert(allLights.end(), lights.begin(), lights.end());
        for (auto& lantern : lanterns) {
            lantern.render(view, projection, allLights, env_light, Camera::position);
        }
        
        // 渲染煙火粒子（半透明，在不透明物體之後）
        auto particleRenderStart = Clock::now();
        firework.render(view, projection);
        
        // 渲染樹（包含半透明葉子，在煙火之後，湖面之前）
        tree1.render(view, projection, lights, env_light, Camera::position);
        tree2.render(view, projection, lights, env_light, Camera::position);
        tree3.render(view, projection, lights, env_light, Camera::position);
        tree4.render(view, projection, lights, env_light, Camera::position);
        tree5.render(view, projection, lights, env_light, Camera::position);
        tree6.render(view, projection, lights, env_light, Camera::position);

        // 渲染湖面 (半透明，最後渲染以顯示正確的反射和透明度)
        auto lakeRenderStart = Clock::now();
        lake.render(view, projection, reflViewProj, lights, env_light, currentFrame, reflTex);
        
        // ===== Bloom 處理 =====
        // 1. 提取亮度 (降解析度提升性能)
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[0]);
        glViewport(0, 0, window_width / 2, window_height / 2);  // 一半解析度
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(brightExtractShader);
        glUniform1i(glGetUniformLocation(brightExtractShader, "image"), 0);
        glUniform1f(glGetUniformLocation(brightExtractShader, "threshold"), 1.0f);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hdrColorBuffer);
        renderQuad();
        
        // 2. 高斯模糊 (ping-pong) - 減少次數提升性能
        bool horizontal = true, first_iteration = true;
        int blur_amount = 5;  // 從 10 降到 5
        glUseProgram(blurShader);
        for (unsigned int i = 0; i < blur_amount; i++) {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            glUniform1i(glGetUniformLocation(blurShader, "horizontal"), horizontal);
            glUniform1i(glGetUniformLocation(blurShader, "image"), 0);
            glBindTexture(GL_TEXTURE_2D, first_iteration ? pingpongBuffer[0] : pingpongBuffer[!horizontal]);
            renderQuad();
            horizontal = !horizontal;
            if (first_iteration) first_iteration = false;
        }
        
        // ===== Tone Mapping 最終合成 =====
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, window_width, window_height);  // 恢復原解析度
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(tonemapShader);
        glUniform1i(glGetUniformLocation(tonemapShader, "hdrBuffer"), 0);
        glUniform1i(glGetUniformLocation(tonemapShader, "bloomBlur"), 1);
        glUniform1f(glGetUniformLocation(tonemapShader, "exposure"), 0.9f);  // 降低曝光讓煙火更清晰
        glUniform1f(glGetUniformLocation(tonemapShader, "bloomStrength"), 0.06f);  // 降低bloom讓細節更清楚
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hdrColorBuffer);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingpongBuffer[!horizontal]);
        renderQuad();

        // 9. 交换缓冲区和轮询事件
        auto bufferSwapStart = Clock::now();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
