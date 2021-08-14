#define VMA_IMPLEMENTATION
#include <vk-engine/vma.h>

VmaAllocator allocator;

bool init_vma(VkPhysicalDevice physDevice, VkDevice device, VkInstance instance) {
    VmaAllocatorCreateInfo createInfo = {};
    createInfo.vulkanApiVersion = VK_API_VERSION_1_2;
    createInfo.physicalDevice = physDevice;
    createInfo.device = device;
    createInfo.instance = instance;

    return vmaCreateAllocator(&createInfo, &allocator) == VK_SUCCESS;
}

void destroy_vma() {
    vmaDestroyAllocator(allocator);
}

VmaAllocator* get_vma() {
    return &allocator;
}