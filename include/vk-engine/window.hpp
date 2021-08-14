#pragma once

#include <GLFW/glfw3.h>

#include <vk-engine/engine.hpp>

namespace unibox {
    class Window {
        GLFWwindow* window;

        Engine engine;
    public:
        Window();
        ~Window();

        bool init();

        bool shouldClose();
        void frameStart();
        void render();

        void waitIdle();

        GLFWwindow* getWindow();
        Engine& getEngine();
    };
}