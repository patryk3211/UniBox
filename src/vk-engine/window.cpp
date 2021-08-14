#include <vk-engine/window.hpp>

#include <spdlog/spdlog.h>

using namespace unibox;

Window::Window() {
    window = 0;
}

Window::~Window() {
    glfwDestroyWindow(window);
    glfwTerminate();
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

    window = glfwCreateWindow(1024, 720, "UniBox", 0, 0);
    if(window == 0) {
        spdlog::error("Could not create a window.\n");
        return false;
    }
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