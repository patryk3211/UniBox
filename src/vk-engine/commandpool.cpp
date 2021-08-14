#include <vk-engine/commandpool.hpp>

#include <spdlog/spdlog.h>

using namespace unibox;

CommandPool::CommandPool(const VkDevice& device, uint32_t queueIndex) {
    this->device = device;
    this->queue = queue;

    VkCommandPoolCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queueIndex
    };
    if(vkCreateCommandPool(device, &createInfo, 0, &handle) != VK_SUCCESS) {
        spdlog::error("Could not create a command pool.");
    }
}

CommandPool::~CommandPool() {
    vkDestroyCommandPool(device, handle, 0);
}

VkCommandPool CommandPool::getHandle() {
    return handle;
}