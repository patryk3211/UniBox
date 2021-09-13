#include <vk-engine/gfxpipeline.hpp>

#include <vk-engine/engine.hpp>

using namespace unibox;

GraphicsPipeline::GraphicsPipeline(VkDevice device, VkRenderPass renderPass) {
    this->device = device;
    this->renderPass = renderPass;

    for(int i = 0; i < 4; i++) descriptorLayout[i] = 0;
    layout = 0;
    handle = 0;

    blendEnable = false;

    descriptorSetAllocate = true;
}

GraphicsPipeline::GraphicsPipeline() {
    this->device = Engine::getInstance()->getDevice();
    this->renderPass = Engine::getInstance()->getRenderPass();

    for(int i = 0; i < 4; i++) descriptorLayout[i] = 0;
    layout = 0;
    handle = 0;

    blendEnable = false;

    descriptorSetAllocate = true;
}

GraphicsPipeline::~GraphicsPipeline() {
    vkDestroyPipeline(device, handle, 0);
    vkDestroyPipelineLayout(device, layout, 0);
    for(int i = 0; i < 4; i++) vkDestroyDescriptorSetLayout(device, descriptorLayout[i], 0);
}

void GraphicsPipeline::enableAlphaBlend() {
    blendEnable = true;
}

void GraphicsPipeline::setDescriptorAllocate(bool value) {
    this->descriptorSetAllocate = value;
}

void GraphicsPipeline::addShader(Shader* shader) {
    shaders.push_back(shader);
}

int GraphicsPipeline::addBinding(size_t stride, VkVertexInputRate rate) {
    VkVertexInputBindingDescription desc{};
    desc.binding = bindings.size();
    desc.inputRate = rate;
    desc.stride = stride;
    bindings.push_back(desc);
    return desc.binding;
}

void GraphicsPipeline::addAttribute(int binding, int location, uint32_t offset, VkFormat format) {
    VkVertexInputAttributeDescription desc{};
    desc.binding = binding;
    desc.location = location;
    desc.format = format;
    desc.offset = offset;
    attributes.push_back(desc);
}

void GraphicsPipeline::addDescriptors(int set, int binding, VkDescriptorType type, size_t count, VkShaderStageFlagBits stage) {
    if(set >= 4 || set < 0) return;
    VkDescriptorSetLayoutBinding desc{};
    desc.binding = binding;
    desc.descriptorCount = count;
    desc.descriptorType = type;
    desc.stageFlags = stage;
    desc.pImmutableSamplers = 0;
    descriptors[set].push_back(desc);
}

void GraphicsPipeline::addPushConstant(size_t offset, size_t size, VkShaderStageFlags stage) {
    VkPushConstantRange range = {
        .stageFlags = stage,
        .offset = offset,
        .size = size
    };
    pushConstants.push_back(range);
}

void GraphicsPipeline::addDepthStencil(bool testEnable, bool writeEnable, VkCompareOp compareOp) {
    this->depthTest = testEnable;
    this->depthWrite = writeEnable;
    this->compareOp = compareOp;
}

bool GraphicsPipeline::assemble(const VkExtent2D& swapChainExtent, VkDescriptorSet (*descriptorSetAllocator)(VkDescriptorSetLayout)) {
    if(handle != 0) return false;
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages(0);
    shaderStages.reserve(shaders.size());

    for(auto shader = shaders.begin(); shader != shaders.end(); shader++) {
        VkPipelineShaderStageCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        Shader* s = *shader;
        info.stage = s->getStage();
        info.module = s->getHandle();
        info.pName = s->getEntryPoint().c_str();
        shaderStages.push_back(info);
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = bindings.size();
    vertexInputInfo.vertexAttributeDescriptionCount = attributes.size();
    vertexInputInfo.pVertexBindingDescriptions = bindings.data();
    vertexInputInfo.pVertexAttributeDescriptions = attributes.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swapChainExtent.width;
    viewport.height = (float) swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE; // TODO: Enable the culling later
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    if(!blendEnable) colorBlendAttachment.blendEnable = VK_FALSE;
    else {
        colorBlendAttachment.blendEnable = VK_TRUE;

        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;

        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    for(int i = 0; i < 4; i++) {
        // Create descriptor layout
        VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo{};
        descriptorLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorLayoutInfo.bindingCount = descriptors[i].size();
        if(descriptors[i].size() > 0) descriptorLayoutInfo.pBindings = descriptors[i].data();
        if(vkCreateDescriptorSetLayout(device, &descriptorLayoutInfo, 0, &descriptorLayout[i]) != VK_SUCCESS) return false;
        // Allocate descriptor set
        if(descriptorSetAllocate) descriptorSet[i] = descriptorSetAllocator(descriptorLayout[i]);
    }

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    pipelineLayoutInfo.pushConstantRangeCount = pushConstants.size();
    if(pushConstants.size() > 0) pipelineLayoutInfo.pPushConstantRanges = pushConstants.data();

    pipelineLayoutInfo.setLayoutCount = 4;
    pipelineLayoutInfo.pSetLayouts = descriptorLayout;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, 0, &layout) != VK_SUCCESS) return false;

    VkPipelineDepthStencilStateCreateInfo depthInfo{};
    depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthInfo.depthTestEnable = depthTest;
    depthInfo.depthWriteEnable = depthWrite;
    depthInfo.depthCompareOp = compareOp;
    depthInfo.depthBoundsTestEnable = VK_FALSE;
    depthInfo.stencilTestEnable = VK_FALSE;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = shaderStages.size();
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = depthTest || depthWrite ? &depthInfo : 0;
    
    pipelineInfo.layout = layout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, 0, &handle);
    if(result == VK_SUCCESS) {
        shaders.clear();
        for(int i = 0; i < 4; i++) descriptors[i].clear();
        pushConstants.clear();
        bindings.clear();
        attributes.clear();
    }
    return result == VK_SUCCESS;
}

VkDescriptorSet GraphicsPipeline::allocateSet(int set) {
    return Engine::getInstance()->allocate_descriptor_set(descriptorLayout[set]);
}

void GraphicsPipeline::bindBufferToDescriptor(int set, int binding, VkBuffer buffer, VkDescriptorType type, size_t offset, size_t length) {
    bindBufferToDescriptor(descriptorSet[set], binding, buffer, type, offset, length);
}

void GraphicsPipeline::bindImageToDescriptor(int set, int binding, VkImageView view, VkSampler sampler, VkImageLayout layout, VkDescriptorType type) {
    bindImageToDescriptor(descriptorSet[set], binding, view, sampler, layout, type);
}

void GraphicsPipeline::bindBufferToDescriptor(VkDescriptorSet set, int binding, VkBuffer buffer, VkDescriptorType type, size_t offset, size_t length) {
    VkDescriptorBufferInfo bufferInfo = {
        .buffer = buffer,
        .offset = offset,
        .range = length
    };
    VkWriteDescriptorSet writeInfo = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,

        .dstSet = set,
        .dstBinding = binding,

        .descriptorCount = 1,
        .descriptorType = type,
        .pBufferInfo = &bufferInfo
    };
    vkUpdateDescriptorSets(device, 1, &writeInfo, 0, 0);
}

void GraphicsPipeline::bindImageToDescriptor(VkDescriptorSet set, int binding, VkImageView view, VkSampler sampler, VkImageLayout layout, VkDescriptorType type) {
    VkDescriptorImageInfo imageInfo = {
        .sampler = sampler,
        .imageView = view,
        .imageLayout = layout
    };
    VkWriteDescriptorSet writeInfo = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,

        .dstSet = set,
        .dstBinding = binding,

        .descriptorCount = 1,
        .descriptorType = type,
        .pImageInfo = &imageInfo
    };
    vkUpdateDescriptorSets(device, 1, &writeInfo, 0, 0);
}

bool GraphicsPipeline::assemble(const VkExtent2D& swapChainExtent) {
    return assemble(swapChainExtent, [](VkDescriptorSetLayout layout) { return Engine::getInstance()->allocate_descriptor_set(layout); });
}

VkPipeline GraphicsPipeline::getHandle() {
    return handle;
}

VkPipelineLayout GraphicsPipeline::getLayout() {
    return layout;
}

VkDescriptorSet* GraphicsPipeline::getDescriptorSet() {
    return descriptorSet;
}
