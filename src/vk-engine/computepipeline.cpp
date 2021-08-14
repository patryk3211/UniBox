#include <vk-engine/computepipeline.hpp>

#include <vk-engine/engine.hpp>

#include <spdlog/spdlog.h>

using namespace unibox;

ComputePipeline::ComputePipeline(const VkDevice& device) {
    this->device = device;

    shader = 0;

    handle = 0;
    layout = 0;
    for(int i = 0; i < 4; i++) descriptorLayout[i] = 0;
}

ComputePipeline::ComputePipeline() {
    this->device = Engine::getInstance()->getDevice();

    shader = 0;

    handle = 0;
    layout = 0;
    for(int i = 0; i < 4; i++) descriptorLayout[i] = 0;
}

ComputePipeline::~ComputePipeline() {
    vkDestroyPipeline(device, handle, 0);
    vkDestroyPipelineLayout(device, layout, 0);
    for(int i = 0; i < 4; i++) vkDestroyDescriptorSetLayout(device, descriptorLayout[i], 0);
}

void ComputePipeline::setShader(Shader* shader) {
    this->shader = shader;
}

void ComputePipeline::addDescriptors(int set, int binding, VkDescriptorType type, size_t count, VkShaderStageFlagBits stage) {
    if(set >= 4 || set < 0) return;
    VkDescriptorSetLayoutBinding desc{};
    desc.binding = binding;
    desc.descriptorCount = count;
    desc.descriptorType = type;
    desc.stageFlags = stage;
    desc.pImmutableSamplers = 0;
    descriptors[set].push_back(desc);
}

void ComputePipeline::addPushConstant(size_t offset, size_t size, VkShaderStageFlags stage) {
    VkPushConstantRange range = {
        .stageFlags = stage,
        .offset = offset,
        .size = size
    };
    pushConstants.push_back(range);
}

bool ComputePipeline::assemble(VkDescriptorSet (*descriptorSetAllocator)(VkDescriptorSetLayout)) {
    if(shader == 0 || handle != 0) return false;

    for(int i = 0; i < 4; i++) {
        // Create descriptor layout
        VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo{};
        descriptorLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorLayoutInfo.bindingCount = descriptors[i].size();
        if(descriptors[i].size() > 0) descriptorLayoutInfo.pBindings = descriptors[i].data();
        if(vkCreateDescriptorSetLayout(device, &descriptorLayoutInfo, 0, &descriptorLayout[i]) != VK_SUCCESS) return false;
        // Allocate descriptor set
        descriptorSet[i] = descriptorSetAllocator(descriptorLayout[i]);
    }

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    pipelineLayoutInfo.pushConstantRangeCount = pushConstants.size();
    if(pushConstants.size() > 0) pipelineLayoutInfo.pPushConstantRanges = pushConstants.data();

    pipelineLayoutInfo.setLayoutCount = 4;
    pipelineLayoutInfo.pSetLayouts = descriptorLayout;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, 0, &layout) != VK_SUCCESS) return false;

    VkComputePipelineCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = shader->getStage(),
            .module = shader->getHandle(),
            .pName = shader->getEntryPoint().c_str()
        },
        .layout = layout
    };

    if(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &createInfo, 0, &handle) != VK_SUCCESS) {
        spdlog::error("Could not create a compute pipeline.");
        return false;
    }

    shader = 0;
    for(int i = 0; i < 4; i++) descriptors[i].clear();
    pushConstants.clear();
    return true;
}

bool ComputePipeline::assemble() {
    return assemble([](VkDescriptorSetLayout layout) { return Engine::getInstance()->allocate_descriptor_set(layout); });
}

bool ComputePipeline::assemble(const std::string& entryPoint) {
    if(shader == 0 || handle != 0) return false;

    for(int i = 0; i < 4; i++) {
        // Create descriptor layout
        VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo{};
        descriptorLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorLayoutInfo.bindingCount = descriptors[i].size();
        if(descriptors[i].size() > 0) descriptorLayoutInfo.pBindings = descriptors[i].data();
        if(vkCreateDescriptorSetLayout(device, &descriptorLayoutInfo, 0, &descriptorLayout[i]) != VK_SUCCESS) return false;
        // Allocate descriptor set
        descriptorSet[i] = Engine::getInstance()->allocate_descriptor_set(descriptorLayout[i]);
    }

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    pipelineLayoutInfo.pushConstantRangeCount = pushConstants.size();
    if(pushConstants.size() > 0) pipelineLayoutInfo.pPushConstantRanges = pushConstants.data();

    pipelineLayoutInfo.setLayoutCount = 4;
    pipelineLayoutInfo.pSetLayouts = descriptorLayout;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, 0, &layout) != VK_SUCCESS) return false;

    VkComputePipelineCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = shader->getStage(),
            .module = shader->getHandle(),
            .pName = entryPoint.c_str()
        },
        .layout = layout
    };

    if(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &createInfo, 0, &handle) != VK_SUCCESS) {
        spdlog::error("Could not create a compute pipeline.");
        return false;
    }

    shader = 0;
    for(int i = 0; i < 4; i++) descriptors[i].clear();
    pushConstants.clear();
    return true;
}

VkPipeline ComputePipeline::getHandle() {
    return handle;
}

VkPipelineLayout ComputePipeline::getLayout() {
    return layout;
}

VkDescriptorSet* ComputePipeline::getDescriptorSet() {
    return descriptorSet;
}

void ComputePipeline::bindBufferToDescriptor(int set, int binding, VkBuffer buffer, VkDescriptorType type, size_t offset, size_t length) {
    VkDescriptorBufferInfo bufferInfo = {
        .buffer = buffer,
        .offset = offset,
        .range = length
    };
    VkWriteDescriptorSet writeInfo = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,

        .dstSet = descriptorSet[set],
        .dstBinding = binding,

        .descriptorCount = 1,
        .descriptorType = type,
        .pBufferInfo = &bufferInfo
    };
    vkUpdateDescriptorSets(device, 1, &writeInfo, 0, 0);
}
