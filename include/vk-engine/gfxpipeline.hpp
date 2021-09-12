#pragma once

#include <list>
#include <iterator>
#include <string>

#include <vk-engine/shader.hpp>

namespace unibox {
    class GraphicsPipeline {
        std::list<Shader*> shaders;

        std::vector<VkVertexInputBindingDescription> bindings;
        std::vector<VkVertexInputAttributeDescription> attributes;

        std::vector<VkDescriptorSetLayoutBinding> descriptors[4];
        std::vector<VkPushConstantRange> pushConstants;
        
        VkRenderPass renderPass;
        VkDevice device;

        VkDescriptorSet descriptorSet[4];
        VkDescriptorSetLayout descriptorLayout[4];
        VkPipelineLayout layout;
        VkPipeline handle;

        bool depthTest;
        bool depthWrite;
        VkCompareOp compareOp;

        bool blendEnable;
    public:
        GraphicsPipeline(VkDevice device, VkRenderPass renderPass);
        GraphicsPipeline();
        ~GraphicsPipeline();

        void addShader(Shader* shader);

        int addBinding(size_t stride, VkVertexInputRate rate);
        void addAttribute(int binding, int location, uint32_t offset, VkFormat format);
        void addDescriptors(int set, int binding, VkDescriptorType type, size_t count, VkShaderStageFlagBits stage);
        void addPushConstant(size_t offset, size_t size, VkShaderStageFlags stage);

        void addDepthStencil(bool testEnable, bool writeEnable, VkCompareOp compareOp);

        void bindBufferToDescriptor(int set, int binding, VkBuffer buffer, VkDescriptorType type, size_t offset, size_t length);

        void enableAlphaBlend();

        bool assemble(const VkExtent2D& swapChainExtent, VkDescriptorSet (*descriptorSetAllocator)(VkDescriptorSetLayout));
        bool assemble(const VkExtent2D& swapChainExtent);

        VkPipeline getHandle();
        VkPipelineLayout getLayout();
        VkDescriptorSet* getDescriptorSet();
    };
}