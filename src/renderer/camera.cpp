#include <renderer/camera.hpp>

#define GLM_FORCE_RADIANS
//#define GLM_FORCE_LEFT_HANDED
#define GLM_DEPTH_ZERO_TO_ONE
#include <glm/trigonometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vk-engine/engine.hpp>

using namespace unibox;

Camera* Camera::instance = 0;

Camera::Camera()
    : buffer(Engine::getInstance()->padUbo(sizeof(glm::mat4)*2), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, VMA_MEMORY_USAGE_CPU_TO_GPU) {
    position = glm::vec3(0, 0, 0);
    rotation = glm::vec3(0, 0, 0);
    projection = glm::mat4();

    instance = this;
}

Camera::~Camera() {
    if(instance == this) instance = 0;
}

void Camera::setPosition(const glm::vec3& position) {
    this->position = position;
}

void Camera::setRotation(const glm::vec3& rotation) {
    this->rotation = rotation;
}

glm::vec3 Camera::getPosition() {
    return position;
}

glm::vec3 Camera::getRotation() {
    return rotation;
}

void Camera::getViewMatrix(glm::mat4* dst) {
    glm::mat4 matrix = glm::translate(glm::mat4(1.0f), position);
    matrix = glm::rotate(matrix, glm::radians(rotation.x), glm::vec3(1, 0, 0));
    matrix = glm::rotate(matrix, glm::radians(rotation.y), glm::vec3(0, 1, 0));
    matrix = glm::rotate(matrix, glm::radians(rotation.z), glm::vec3(0, 0, 1));
    *dst = glm::inverse(matrix);
}

glm::mat4 Camera::getProjectionMatrix() {
    return projection;
}

void Camera::perspective(float fov, float aspect) {
    projection = glm::perspective(glm::radians(fov), aspect, 0.1f, 1000.0f);
    projection[1][0] = -projection[1][0];
    projection[1][1] = -projection[1][1];
    projection[1][2] = -projection[1][2];
    projection[1][3] = -projection[1][3];
}

void Camera::orthographic(float aspect, float zNear, float zFar, float scale) {
    projection = glm::ortho(-aspect*scale, aspect*scale, -1.0f*scale, 1.0f*scale, zNear, zFar);
    projection[1][0] = -projection[1][0];
    projection[1][1] = -projection[1][1];
    projection[1][2] = -projection[1][2];
    projection[1][3] = -projection[1][3];
}

void Camera::updateBuffer() {
    struct MatrixPair {
        glm::mat4 view;
        glm::mat4 proj;
    } data;
    getViewMatrix(&data.view);
    data.proj = projection;
    buffer.store(&data, 0, sizeof(MatrixPair));
}

Buffer& Camera::getBuffer() {
    return buffer;
}

Camera* Camera::getInstance() {
    return instance;
}