#pragma once

#include <vulkan/vulkan.h>

#include <vk-engine/shader.hpp>

#include <vector>

namespace unibox {
    class ComputePipeline {
        VkDevice device;

        VkPipeline handle;
        VkPipelineLayout layout;
        VkDescriptorSet descriptorSet[4];
        VkDescriptorSetLayout descriptorLayout[4];

        std::vector<VkDescriptorSetLayoutBinding> descriptors[4];
        std::vector<VkPushConstantRange> pushConstants;

        Shader* shader;
    public:
        ComputePipeline(const VkDevice& device);
        ComputePipeline();
        ~ComputePipeline();

        void setShader(Shader* shader);
        void addDescriptors(int set, int binding, VkDescriptorType type, size_t count, VkShaderStageFlagBits stage);
        void addPushConstant(size_t offset, size_t size, VkShaderStageFlags stage);

        void bindBufferToDescriptor(int set, int binding, VkBuffer buffer, VkDescriptorType type, size_t offset, size_t length);

        bool assemble(VkDescriptorSet (*descriptorSetAllocator)(VkDescriptorSetLayout));
        bool assemble();
        bool assemble(const std::string& entryPoint);

        VkPipeline getHandle();
        VkPipelineLayout getLayout();
        VkDescriptorSet* getDescriptorSet();
    };
}