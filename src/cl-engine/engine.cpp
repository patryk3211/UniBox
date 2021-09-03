#include <cl-engine/engine.hpp>

#include <spdlog/spdlog.h>

using namespace unibox;

ClEngine* ClEngine::instance = 0;

ClEngine::ClEngine() {
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);

    valid = false;

    for(auto& platform : platforms) {
        std::vector<cl::Device> devices;
        platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
        for(auto& device : devices) {
            // Select the first available device.
            this->device = device;
            valid = true;
            break;
        }
        if(valid) break;
    }

    if(!valid) {
        spdlog::error("Could not get a valid OpenCL Device!");
        return;
    }

    context = cl::Context(device);

    instance = this;
}

ClEngine::~ClEngine() {

}

cl::Context& ClEngine::getContext() {
    return context;
}

cl::Device& ClEngine::getDevice() {
    return device;
}

ClEngine* ClEngine::getInstance() {
    return instance;
}
