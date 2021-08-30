#pragma once

#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <VkBootstrap.h>

#include <vector>
#include <list>
#include <thread>
#include <unordered_map>
#include <mutex>

#include <vk-engine/commandpool.hpp>
#include <vk-engine/commandbuffer.hpp>
#include <vk-engine/gfxpipeline.hpp>
#include <vk-engine/buffer.hpp>

#include <util/semaphore.h>

namespace unibox {
    enum QueueType {
        GRAPHICS,
        COMPUTE,
        TRANSFER,
        PRESENT
    };

    class Engine {
        static Engine* instance;

        VkDevice device;
        vkb::Device vkb_device;
        vkb::PhysicalDevice vkb_physDevice;
        vkb::Instance vkb_instance;

        VkSurfaceKHR surface;

        VkQueue gfx_queue;
        VkQueue present_queue;
        uint32_t gfx_queue_index;
        uint32_t present_queue_index;

        VkQueue compute_queue;
        uint32_t compute_queue_index;

        vkb::Swapchain vkb_swapchain;
        std::vector<VkImageView> imageViews;

        /*std::list<CommandPool*> gfx_cmd_pools;
        std::list<CommandBuffer*> gfx_buffers;

        std::list<CommandPool*> comp_cmd_pools;
        std::list<CommandBuffer*> comp_buffers;*/

        CommandPool* default_gfx_pool;
        CommandPool* default_comp_pool;
        CommandBuffer* default_gfx_buffer;

        std::unordered_map<CommandPool*, std::list<CommandBuffer*>> cmd_buffers;
        std::unordered_map<CommandBuffer*, CommandPool*> cmd_buffer_pools;

        VkRenderPass renderpass;
        std::vector<VkFramebuffer> framebuffers;

        uint32_t width;
        uint32_t height;

        VkSemaphore presentSemaphore, renderSemaphore;
        VkFence renderFence;

        VkDescriptorPool lastPool;
        std::list<VkDescriptorPool> pools;

        std::vector<void (*)(VkCommandBuffer)> renderCalls;

        bool init_swapchain(uint32_t width, uint32_t height);
        bool init_commands();
        bool init_renderpass();
        bool init_framebuffers();
        bool init_sync_structures();

        VkDescriptorPool create_descriptor_pool();

        std::recursive_mutex descriptorAllocatorLock;

    public:
        Engine();
        ~Engine();

        void waitIdle();

        bool init(GLFWwindow* window);
        void draw();

        VkDescriptorSet allocate_descriptor_set(VkDescriptorSetLayout layout);

        VkDevice getDevice() { return device; }
        VkRenderPass getRenderPass() { return renderpass; }

        void addRenderFunction(void (*renderFunc)(VkCommandBuffer));

        bool isRendering();

        CommandBuffer* allocateBuffer(QueueType type);
        CommandBuffer* allocateBuffer(CommandPool* pool);
        CommandPool* allocatePool(QueueType type);

        void freeBuffer(CommandBuffer* buffer);
        void freePool(CommandPool* pool);

        VkQueue getQueue(QueueType type);

        static Engine* getInstance() { return instance; }

        size_t padUbo(size_t bufferSize);
    };
}