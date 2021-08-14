#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include <vk-engine/buffer.hpp>

namespace unibox {
    class Camera {
        static Camera* instance;

        glm::vec3 position;
        glm::vec3 rotation;

        glm::mat4 projection;

        Buffer buffer;
    public:
        Camera();
        ~Camera();

        void perspective(float fov, float aspect);
        void orthographic(float aspect, float zNear, float zFar, float scale);

        void setPosition(const glm::vec3& position);
        void setRotation(const glm::vec3& rotation);

        glm::vec3 getPosition();
        glm::vec3 getRotation();

        void getViewMatrix(glm::mat4* dst);
        glm::mat4 getProjectionMatrix();

        void updateBuffer();
        Buffer& getBuffer();

        static Camera* getInstance();
    };
}