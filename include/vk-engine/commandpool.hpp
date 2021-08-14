#pragma once

#include <vulkan/vulkan.h>

namespace unibox {
    class CommandPool {
        VkDevice device;
        VkQueue queue;

        VkCommandPool handle;
    public:
        CommandPool(const VkDevice& device, uint32_t queueIndex);
        ~CommandPool();

        VkCommandPool getHandle();
    };
}