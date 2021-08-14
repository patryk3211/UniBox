#pragma once

#include <vulkan/vulkan.h>
#include <vk-engine/vk_mem_alloc.h>

bool init_vma(VkPhysicalDevice physDevice, VkDevice device, VkInstance instance);
void destroy_vma();
VmaAllocator* get_vma();