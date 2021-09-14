#pragma once

#include <GLFW/glfw3.h>

#include <vk-engine/engine.hpp>
#include <glm/vec2.hpp>

#include <algorithm>
#include <list>

namespace unibox {
    class Window {
        static Window* instance;
        GLFWwindow* window;

        Engine engine;
    
        std::list<std::function<void(double, double, int)>> mouseDownCallbacks;
        std::list<std::function<void(double, double, int)>> mouseUpCallbacks;

        static void mouseButtonCallback(GLFWwindow* window, int, int, int);
    public:
        Window();
        ~Window();

        bool init();

        bool shouldClose();
        void frameStart();
        void render();

        void waitIdle();

        glm::vec2 getCursorPos();

        GLFWwindow* getWindow();
        Engine& getEngine();

        void addMouseDownCallback(std::function<void(double, double, int)> callback);
        void addMouseUpCallback(std::function<void(double, double, int)> callback);
    };
}