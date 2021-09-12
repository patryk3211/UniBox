#include <vk-engine/buffer.hpp>

#include <vk-engine/engine.hpp>

#include <spdlog/spdlog.h>

using namespace unibox;

Buffer::Buffer(size_t size, VkBufferUsageFlags usage, VkSharingMode share, VmaMemoryUsage memType) {
    this->handle = 0;
    this->allocation = 0;

    this->size = size;

    VkBufferCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = share
    };
    VmaAllocationCreateInfo allocInfo = {
        .usage = memType
    };

    if(vmaCreateBuffer(*get_vma(), &createInfo, &allocInfo, &handle, &allocation, 0) != VK_SUCCESS)
        spdlog::error("Could not allocate a buffer.");
}

Buffer::~Buffer() {
    vmaDestroyBuffer(*get_vma(), handle, allocation);
}

void Buffer::store(const void* source, uint32_t offset, size_t length) {
    void* buffer;
    size_t copyAmount = length;
    if(copyAmount+offset > size) {
        spdlog::error("Tried to store more data than the buffer was constructed for, storing the max allowed amount.");
        copyAmount = size-offset;
    }
    vmaMapMemory(*get_vma(), allocation, &buffer);
    memcpy(buffer, source, copyAmount);
    vmaUnmapMemory(*get_vma(), allocation);
}

void Buffer::load(void* dest, uint32_t offset, size_t length) {
    void* buffer;
    size_t copyAmount = length;
    if(copyAmount+offset > size) {
        spdlog::error("Tried to store more data than the buffer was constructed for, storing the max allowed amount.");
        copyAmount = size-offset;
    }
    vmaMapMemory(*get_vma(), allocation, &buffer);
    memcpy(dest, buffer, copyAmount);
    vmaUnmapMemory(*get_vma(), allocation);
}

void* Buffer::map() {
    void* buffer;
    vmaMapMemory(*get_vma(), allocation, &buffer);
    return buffer;
}

void Buffer::unmap() {
    vmaUnmapMemory(*get_vma(), allocation);
}

VkBuffer& Buffer::getHandle() {
    return handle;
}