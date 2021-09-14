#include <vk-engine/window.hpp>

#include <spdlog/spdlog.h>

using namespace unibox;

Window* Window::instance = 0;

Window::Window() {
    window = 0;
    instance = this;
}

Window::~Window() {
    glfwDestroyWindow(window);
    glfwTerminate();
    instance = 0;
}

void Window::waitIdle() {
    engine.waitIdle();
}

bool Window::init() {
    if(!glfwInit()) {
        spdlog::error("Could not initialize GLFW.\n");
        return false;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(1280, 720, "UniBox", 0, 0);
    if(window == 0) {
        spdlog::error("Could not create a window.\n");
        return false;
    }
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    spdlog::info("Initializing render engine.");
    return engine.init(window);
}

GLFWwindow* Window::getWindow() {
    return window;
}

Engine& Window::getEngine() {
    return engine;
}

bool Window::shouldClose() {
    return glfwWindowShouldClose(window);
}

void Window::frameStart() {
    glfwPollEvents();
}

void Window::render() {
    engine.draw();
}

glm::vec2 Window::getCursorPos() {
    double xPos;
    double yPos;
    glfwGetCursorPos(window, &xPos, &yPos);
    return glm::vec2(xPos, yPos);
}

void Window::mouseButtonCallback(GLFWwindow* window, int button, int eventType, int other) {
    if(instance->window == window) {
        glm::vec2 mousePos = instance->getCursorPos();
        if(eventType == 1) {
            for(auto& call : instance->mouseDownCallbacks) {
                call(mousePos.x, mousePos.y, button);
            }
        } else if (eventType == 0) {
            for(auto& call : instance->mouseUpCallbacks) {
                call(mousePos.x, mousePos.y, button);
            }
        }
    }
}

void Window::addMouseDownCallback(std::function<void(double, double, int)> callback) {
    this->mouseDownCallbacks.push_back(callback);
}

void Window::addMouseUpCallback(std::function<void(double, double, int)> callback) {
    this->mouseUpCallbacks.push_back(callback);
}
