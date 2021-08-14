#pragma once

#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <VkBootstrap.h>

#include <vector>
#include <list>
#include <thread>

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

        std::list<CommandPool*> gfx_cmd_pools;
        std::list<CommandBuffer*> gfx_buffers;

        std::list<CommandPool*> comp_cmd_pools;
        std::list<CommandBuffer*> comp_buffers;

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

        CommandBuffer* allocateBuffer(QueueType type, bool fromNewPool);
        void freeBuffer(CommandBuffer* buffer);
        VkQueue getQueue(QueueType type);

        static Engine* getInstance() { return instance; }

        size_t padUbo(size_t bufferSize);
    };
}