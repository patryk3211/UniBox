#include <vk-engine/image.hpp>

#include <vk-engine/buffer.hpp>
#include <vk-engine/commandpool.hpp>
#include <vk-engine/commandbuffer.hpp>
#include <vk-engine/engine.hpp>

#include <spdlog/spdlog.h>

using namespace unibox;

Image::Image(unsigned int width, unsigned int height, VkImageUsageFlags usage, VkSharingMode sharing, VkFormat format, VkImageTiling tiling) {
    this->device = Engine::getInstance()->getDevice();
    
    this->handle = 0;
    this->memory = 0;

    this->width = width;
    this->height = height;

    this->format = format;

    this->view = 0;
    this->sampler = 0;

    VkImageCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = { width, height, 1 },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = tiling,
        .usage = usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .sharingMode = sharing,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };
    VmaAllocationCreateInfo allocInfo = {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY
    };

    if(vmaCreateImage(*get_vma(), &createInfo, &allocInfo, &handle, &memory, 0) != VK_SUCCESS) 
        spdlog::error("Failed to create an image.");
}

Image::~Image() {
    if(sampler != 0) vkDestroySampler(device, sampler, 0);
    if(view != 0) vkDestroyImageView(device, view, 0);
    vmaDestroyImage(*get_vma(), handle, memory);
}

void Image::copyToImage(VkBuffer buffer, VkCommandBuffer cmd) {
    VkBufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
        .imageOffset = { 0, 0, 0 },
        .imageExtent = { width, height, 1 }
    };
    vkCmdCopyBufferToImage(cmd, buffer, handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void Image::loadImage(const void* data, size_t length) {
    Buffer srcBuf = Buffer(length, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE, VMA_MEMORY_USAGE_CPU_ONLY);
    srcBuf.store(data, 0, length);

    VkFence fence;
    VkSemaphore semaphore;
    VkDevice device = Engine::getInstance()->getDevice();
    {
        VkFenceCreateInfo info = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        if(vkCreateFence(device, &info, 0, &fence) != VK_SUCCESS) {
            spdlog::error("Failed to create image load fence.");
            return;
        }
        VkSemaphoreCreateInfo info2 = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        if(vkCreateSemaphore(device, &info2, 0, &semaphore) != VK_SUCCESS) {
            spdlog::error("Failed to create image load semaphore.");
            return;
        }
    }

    CommandPool* pool = Engine::getInstance()->allocatePool(QueueType::TRANSFER);
    CommandBuffer* cmd1 = Engine::getInstance()->allocateBuffer(pool);
    if(cmd1->startRecording()) { // Transition to transfer optimal.
        transitionLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, cmd1->getHandle());
        cmd1->stopRecording();
        cmd1->submit(Engine::getInstance()->getQueue(QueueType::TRANSFER), {}, { }, 0, 0);
    }
    CommandBuffer* cmd2 = Engine::getInstance()->allocateBuffer(pool);
    if(cmd2->startRecording()) { // Load image data.
        copyToImage(srcBuf.getHandle(), cmd2->getHandle());
        cmd2->stopRecording();
        cmd2->submit(Engine::getInstance()->getQueue(QueueType::TRANSFER), { }, { }, 0, 0);
    }
    CommandBuffer* cmd3 = Engine::getInstance()->allocateBuffer(pool);
    if(cmd3->startRecording()) { // Transfer to graphics queue.
        transitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         VK_ACCESS_TRANSFER_WRITE_BIT, 0,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         Engine::getInstance()->getQueueIndex(QueueType::TRANSFER), Engine::getInstance()->getQueueIndex(QueueType::GRAPHICS), cmd3->getHandle());
        cmd3->stopRecording();
        cmd3->submit(Engine::getInstance()->getQueue(QueueType::TRANSFER), { }, { semaphore }, 0, 0);
    }
    CommandBuffer* cmd4 = Engine::getInstance()->allocateBuffer(Engine::getInstance()->allocatePool(QueueType::GRAPHICS));
    if(cmd4->startRecording()) {
        transitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                         0, VK_ACCESS_SHADER_READ_BIT,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, cmd4->getHandle());
        cmd4->stopRecording();
        cmd4->submit(Engine::getInstance()->getQueue(QueueType::GRAPHICS), { semaphore }, { }, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, fence);
    }

    vkWaitForFences(device, 1, &fence, VK_TRUE, 0xFFFFFFFF);

    Engine::getInstance()->freeBuffer(cmd1);
    Engine::getInstance()->freeBuffer(cmd2);
    Engine::getInstance()->freeBuffer(cmd3);
    Engine::getInstance()->freeBuffer(cmd4);

    vkDestroyFence(device, fence, 0);
    vkDestroySemaphore(device, semaphore, 0);
}

void Image::transitionLayout(VkImageLayout srcLayout, VkImageLayout dstLayout, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkCommandBuffer cmd) {
    transitionLayout(srcLayout, dstLayout, srcAccess, dstAccess, srcStage, dstStage, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, cmd);
}

void Image::transitionLayout(VkImageLayout srcLayout, VkImageLayout dstLayout, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, uint32_t srcQueue, uint32_t dstQueue, VkCommandBuffer cmd) {
    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,

        .srcAccessMask = srcAccess,
        .dstAccessMask = dstAccess,

        .oldLayout = srcLayout,
        .newLayout = dstLayout,

        .srcQueueFamilyIndex = srcQueue,
        .dstQueueFamilyIndex = dstQueue,

        .image = handle,

        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, 0, 0, 0, 1, &barrier);
}

VkImage& Image::getHandle() {
    return handle;
}

VkImageView& Image::getImageView() {
    if(view == 0) {
        VkImageViewCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = handle,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = format,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        vkCreateImageView(device, &info, 0, &view);
    }
    return view;
}

void Image::createSampler(VkFilter minFilter, VkFilter magFilter, VkSamplerAddressMode uMode, VkSamplerAddressMode vMode, VkSamplerAddressMode wMode) {
    if(sampler != 0) vkDestroySampler(device, sampler, 0);
    VkSamplerCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = magFilter,
        .minFilter = minFilter,

        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,

        .addressModeU = uMode,
        .addressModeV = vMode,
        .addressModeW = wMode,

        .mipLodBias = 0,

        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = Engine::getInstance()->getProperties().limits.maxSamplerAnisotropy,

        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,

        .minLod = 0,
        .maxLod = 0,

        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE
    };

    if(vkCreateSampler(device, &info, 0, &sampler) != VK_SUCCESS)
        spdlog::error("Failed to create an image sampler.");
}

VkSampler& Image::getSampler() {
    return sampler;
}
