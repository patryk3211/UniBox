#pragma once

#include <vulkan/vulkan.h>

#include <string>
#include <vector>

namespace unibox {
    class Shader {
        VkDevice device;
        VkShaderStageFlagBits type;

        VkShaderModule handle;

        std::vector<uint32_t> shaderCode;

        std::string entryPoint;
    public:
        Shader(VkDevice device, VkShaderStageFlagBits type, const std::string& entryPoint);
        Shader(VkShaderStageFlagBits type, const std::string& entryPoint);
        ~Shader();

        bool addCode(size_t codeSize, const uint32_t* spirv);
        bool addCode(const std::string& file);

        std::string& getEntryPoint();

        VkShaderModule getHandle();

        VkShaderStageFlagBits getStage();
    };
}