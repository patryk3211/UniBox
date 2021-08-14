#include <compute/simulator.hpp>

#include <vk-engine/engine.hpp>

#include <spdlog/spdlog.h>

using namespace unibox;

#define GROUP_SIZE_X 256

#define UBO_SIZE sizeof(uint)*3

#define VERTEX_SIZE sizeof(uint)*4+sizeof(float)*4

Simulator::Simulator()
    : simulatePipeline(),
      resetPipeline(),
      ubo(UBO_SIZE, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, VMA_MEMORY_USAGE_CPU_TO_GPU) {
    device = Engine::getInstance()->getDevice();

    Shader shader = Shader(VK_SHADER_STAGE_COMPUTE_BIT, "main");
    shader.addCode("shaders/compute/simulator.spv");

    simulatePipeline.setShader(&shader);
    simulatePipeline.addDescriptors(0, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT);
    simulatePipeline.addDescriptors(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT);

    if(!simulatePipeline.assemble()) {
        spdlog::error("Could not assemble the Simulation Pipeline.");
        return;
    }

    Shader shader2 = Shader(VK_SHADER_STAGE_COMPUTE_BIT, "main");
    shader2.addCode("shaders/compute/resetter.spv");

    resetPipeline.setShader(&shader2);
    resetPipeline.addDescriptors(0, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT);
    resetPipeline.addDescriptors(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT);

    if(!resetPipeline.assemble()) {
        spdlog::error("Could not assemble the Simulation Reset Pipeline.");
        return;
    }

    cmd = Engine::getInstance()->allocateBuffer(QueueType::COMPUTE, true);

    VkFenceCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    if(vkCreateFence(device, &info, 0, &fence) != VK_SUCCESS) {
        spdlog::error("Could not create a synchronization fence in Simulator.");
    }

    resetPipeline.bindBufferToDescriptor(0, 1, ubo.getHandle(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, UBO_SIZE);
    simulatePipeline.bindBufferToDescriptor(0, 1, ubo.getHandle(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, UBO_SIZE);
}

Simulator::~Simulator() {
    Engine::getInstance()->freeBuffer(cmd);
    vkDestroyFence(device, fence, 0);
}

void Simulator::simulate(uint32_t sizeX, uint32_t sizeY, uint32_t sizeZ, VkBuffer particleBuffer) {
    if(vkWaitForFences(device, 1, &fence, true, 0xFFFFFFFF) != VK_SUCCESS) return;
    if(vkResetFences(device, 1, &fence) != VK_SUCCESS) return;

    if(!cmd->resetBuffer()) return;
    if(!cmd->startRecording()) return;
    VkCommandBuffer handle = cmd->getHandle();

    uint32_t count = sizeX*sizeY*sizeZ;
    uint32_t groupCount = count/GROUP_SIZE_X+(count%GROUP_SIZE_X==0?0:1);

    struct {
        uint sizeX;
        uint sizeY;
        uint sizeZ;
    } uboData;
    uboData.sizeX = sizeX;
    uboData.sizeY = sizeY;
    uboData.sizeZ = sizeZ;
    ubo.store(&uboData, 0, sizeof(uboData));

    resetPipeline.bindBufferToDescriptor(0, 0, particleBuffer, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, (VERTEX_SIZE)*groupCount*GROUP_SIZE_X);
    simulatePipeline.bindBufferToDescriptor(0, 0, particleBuffer, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, (VERTEX_SIZE)*groupCount*GROUP_SIZE_X);

    vkCmdBindPipeline(handle, VK_PIPELINE_BIND_POINT_COMPUTE, resetPipeline.getHandle());
    vkCmdBindDescriptorSets(handle, VK_PIPELINE_BIND_POINT_COMPUTE, resetPipeline.getLayout(), 0, 1, resetPipeline.getDescriptorSet(), 0, 0);

    vkCmdDispatch(handle, groupCount, 1, 1);

    vkCmdBindPipeline(handle, VK_PIPELINE_BIND_POINT_COMPUTE, simulatePipeline.getHandle());

    vkCmdDispatch(handle, groupCount, 1, 1);

    cmd->stopRecording();

    cmd->submit(Engine::getInstance()->getQueue(QueueType::COMPUTE), {}, {}, 0, fence);
}

bool Simulator::isExecuting() {

}
