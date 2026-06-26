#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

// 湖邊場景 - 攝影機靠近湖岸，大天空小場景
glm::vec3 Camera::position = glm::vec3(35.0f, 2.0f, 35.0f);  // 靠近湖面，低視角
glm::vec3 Camera::front = glm::vec3(-0.7f, 0.0f, -0.7f);     // 望向湖面中心
glm::vec3 Camera::up = glm::vec3(0.0f, 1.0f, 0.0f);

float Camera::yaw = -135.0f;  // 偏航角望向湖面
float Camera::pitch = 5.0f;   // 稍微仰視，看到更多天空

glm::mat4 Camera::getViewMatrix() {
    return glm::lookAt(position, position + front, up);
}

void Camera::move(const glm::vec3& offset) {
    position += offset;
    position = glm::vec3(position.x, glm::max(0.25f, position.y), position.z);
}

void Camera::adjustYaw(float offset) {
    yaw += offset;
    updateCameraVectors();
}

void Camera::adjustPitch(float offset) {
    pitch += offset;

    // 限制俯仰角范围
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    updateCameraVectors();
}

void Camera::updateCameraVectors() {
    // 更新摄像机方向向量
    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(direction);
}
