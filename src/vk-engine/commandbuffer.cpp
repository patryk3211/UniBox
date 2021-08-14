#include <vk-engine/commandbuffer.hpp>

#include <spdlog/spdlog.h>

using namespace unibox;

CommandBuffer::CommandBuffer(const VkDevice& device, const VkCommandPool& pool) {
    this->device = device;
    this->pool = pool;

    VkCommandBufferAllocateInfo allocInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    vkAllocateCommandBuffers(device, &allocInfo, &handle);
}

CommandBuffer::~CommandBuffer() {
    vkFreeCommandBuffers(device, pool, 1, &handle);
}

bool CommandBuffer::startRecording() {
    VkCommandBufferBeginInfo info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
    };
    VkResult res = vkBeginCommandBuffer(handle, &info);
    if(res != VK_SUCCESS) spdlog::error("Failed to start recording a command buffer.");
    return res == VK_SUCCESS;
}

void CommandBuffer::stopRecording() {
    vkEndCommandBuffer(handle);
}

bool CommandBuffer::resetBuffer() {
    VkResult res = vkResetCommandBuffer(handle, 0);
    if(res != VK_SUCCESS) spdlog::error("Failed to reset a command buffer.");
    return res == VK_SUCCESS;
}

bool CommandBuffer::submit(VkQueue queue, std::vector<VkSemaphore> waitSemaphores, std::vector<VkSemaphore> signalSemaphores, VkPipelineStageFlags waitStage, VkFence fence) {
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,

        .waitSemaphoreCount = waitSemaphores.size(),
        .pWaitSemaphores = waitSemaphores.data(),

        .pWaitDstStageMask = &waitStage,

        .commandBufferCount = 1,
        .pCommandBuffers = &handle,

        .signalSemaphoreCount = signalSemaphores.size(),
        .pSignalSemaphores = signalSemaphores.data()
    };

    return vkQueueSubmit(queue, 1, &submitInfo, fence) == VK_SUCCESS;
}

VkCommandBuffer CommandBuffer::getHandle() {
    return handle;
}