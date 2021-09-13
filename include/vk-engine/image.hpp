#pragma once

#include <vulkan/vulkan.h>
#include <vk-engine/vma.h>

#include <string>

namespace unibox {
    class Image {
        VkDevice device;

        VkImage handle;
        VmaAllocation memory;

        VkImageView view;
        VkSampler sampler;

        unsigned int width;
        unsigned int height;

        VkFormat format;

        void copyToImage(VkBuffer buffer, VkCommandBuffer cmd);
    public:
        Image(unsigned int width, unsigned int height, VkImageUsageFlags usage, VkSharingMode sharing, VkFormat format, VkImageTiling tiling);
        ~Image();

        void loadImage(const void* data, size_t length);

        void transitionLayout(VkImageLayout srcLayout, VkImageLayout dstLayout, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkCommandBuffer cmd);
        void transitionLayout(VkImageLayout srcLayout, VkImageLayout dstLayout, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, uint32_t srcQueue, uint32_t dstQueue, VkCommandBuffer cmd);

        VkImage& getHandle();
        VkImageView& getImageView();

        void createSampler(VkFilter minFilter, VkFilter magFilter, VkSamplerAddressMode uMode, VkSamplerAddressMode vMode, VkSamplerAddressMode wMode);
        VkSampler& getSampler();
    };
}