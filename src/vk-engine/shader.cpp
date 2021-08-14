#include <vk-engine/shader.hpp>

#include <istream>
#include <fstream>

#include <spdlog/spdlog.h>

#include <vk-engine/engine.hpp>

using namespace unibox;

VkShaderModule createShader(VkDevice device, size_t codeSize, const uint32_t* spirv) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = codeSize;
    createInfo.pCode = spirv;
    createInfo.flags = 0;
    createInfo.pNext = 0;

    VkShaderModule shader;
    if(vkCreateShaderModule(device, &createInfo, 0, &shader) != VK_SUCCESS) {
        spdlog::error("Failed to create a shader module.");
        return 0;
    }
    return shader;
}

Shader::Shader(VkDevice device, VkShaderStageFlagBits type, const std::string& entryPoint) {
    this->device = device;
    this->type = type;

    this->handle = 0;

    this->entryPoint = entryPoint;
}

Shader::Shader(VkShaderStageFlagBits type, const std::string& entryPoint) {
    this->device = Engine::getInstance()->getDevice();
    this->type = type;

    this->handle = 0;

    this->entryPoint = entryPoint;
}

Shader::~Shader() {
    vkDestroyShaderModule(device, handle, 0);
}

bool Shader::addCode(size_t codeSize, const uint32_t* spirv) {
    handle = createShader(device, codeSize, spirv);
    return handle != 0;
}

bool Shader::addCode(const std::string& file) {
    std::ifstream fileStream(file, std::ios::ate | std::ios::binary);
    if(!fileStream.is_open()) {
        spdlog::error("could not open shader file '" + file + "'.");
        return false;
    }

    size_t length = fileStream.tellg();
    fileStream.seekg(0);

    shaderCode = std::vector<uint32_t>(length/4);

    fileStream.read(reinterpret_cast<char*>(shaderCode.data()), length);
    fileStream.close();

    return addCode(length, shaderCode.data());
}

VkShaderModule Shader::getHandle() {
    return handle;
}

std::string& Shader::getEntryPoint() {
    return entryPoint;
}

VkShaderStageFlagBits Shader::getStage() {
    return type;
}