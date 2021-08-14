#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace unibox {
    class CommandBuffer {
        VkDevice device;
        VkCommandPool pool;

        VkCommandBuffer handle;
    public:
        CommandBuffer(const VkDevice& device, const VkCommandPool& pool);
        ~CommandBuffer();

        bool startRecording();
        void stopRecording();

        bool resetBuffer();

        bool submit(VkQueue queue, std::vector<VkSemaphore> waitSemaphores, std::vector<VkSemaphore> signalSemaphores, VkPipelineStageFlags waitStage, VkFence fence);

        VkCommandBuffer getHandle();

        VkCommandPool getPool() { return pool; }
    };
}