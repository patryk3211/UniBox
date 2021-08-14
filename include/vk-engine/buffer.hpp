#pragma once

#include <vulkan/vulkan.h>

#include <vk-engine/vma.h>

namespace unibox {
    class Buffer {
        VkBuffer handle;
        VmaAllocation allocation;

        size_t size;
    public:
        Buffer(size_t size, VkBufferUsageFlags usage, VkSharingMode share, VmaMemoryUsage memType);
        ~Buffer();

        void store(void* source, uint32_t offset, size_t length);
        void load(void* dest, uint32_t offset, size_t length);

        void* map();
        void unmap();

        VkBuffer& getHandle();
    };
}