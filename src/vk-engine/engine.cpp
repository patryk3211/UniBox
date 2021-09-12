#include <vk-engine/engine.hpp>

#include <spdlog/spdlog.h>
#include <vk-engine/vma.h>

#include <renderer/renderer.hpp>

using namespace unibox;
using namespace vkb;

Engine* Engine::instance = 0;

Engine::Engine() {
    device = 0;

    gfx_queue = 0;
    present_queue = 0;

    renderpass = 0;

    renderSemaphore = 0;
    presentSemaphore = 0;
    renderFence = 0;

    lastPool = 0;

    instance = this;
}

Engine::~Engine() {
    vkDestroyFence(device, renderFence, 0);
    vkDestroySemaphore(device, renderSemaphore, 0);
    vkDestroySemaphore(device, presentSemaphore, 0);

    for(auto pool = pools.begin(); pool != pools.end(); pool++) {
        vkResetDescriptorPool(device, *pool, 0);
        vkDestroyDescriptorPool(device, *pool, 0);
    }

    for(int i = 0; i < framebuffers.size(); i++) vkDestroyFramebuffer(device, framebuffers.data()[i], 0);

    vkDestroyRenderPass(device, renderpass, 0);

    for(auto pool : cmd_buffers) {
        for(auto buffer : pool.second) delete buffer;
        delete pool.first;
    }

    destroy_vma();

    vkb_swapchain.destroy_image_views(imageViews);
    destroy_swapchain(vkb_swapchain);
    destroy_surface(vkb_instance, surface);
    destroy_device(vkb_device);
    destroy_instance(vkb_instance);
}

void Engine::waitIdle() {
    vkDeviceWaitIdle(device);
}

bool Engine::isRendering() {
    VkResult res = vkGetFenceStatus(device, renderFence);
    if(res != VK_NOT_READY && res != VK_SUCCESS) spdlog::error("Could not get status of render fence.");
    return res == VK_NOT_READY;
}

bool Engine::init(GLFWwindow* window) {
    InstanceBuilder instanceBuilder;
    instanceBuilder.set_app_name("UniBox")
                    .set_app_version(1, 0, 0)
                    .set_engine_name("UniBoxEngine")
                    .set_engine_version(1, 0, 0)
                    .require_api_version(1, 1)
                    .use_default_debug_messenger()
                    .request_validation_layers();
    uint32_t count;
    const char** exts = glfwGetRequiredInstanceExtensions(&count);
    for(int i = 0; i < count; i++) instanceBuilder.enable_extension(exts[i]);

    auto inst_ret = instanceBuilder.build();
    
    if(!inst_ret) {
        spdlog::error("Could not create a Vulkan instance.");
        spdlog::error(inst_ret.error().message());
        return false;
    }

    vkb_instance = inst_ret.value();

    if(glfwCreateWindowSurface(vkb_instance.instance, window, 0, &surface) != VK_SUCCESS) {
        spdlog::error("Could not create a window surface.");
        return false;
    }

    PhysicalDeviceSelector selector { vkb_instance };
    auto phys_ret = selector.set_surface(surface)
                            .set_minimum_version(1, 0)
                            .select();
    if(!phys_ret) {
        spdlog::error("Could not select a physical device.");
        spdlog::error(phys_ret.error().message());
        return false;
    }
    
    vkb_physDevice = phys_ret.value();
    DeviceBuilder devBuilder { vkb_physDevice };
    auto dev_ret = devBuilder.build();
    if(!dev_ret) {
        spdlog::error("Could not create a logical device.");
        spdlog::error(dev_ret.error().message());
        return false;
    }

    vkb_device = dev_ret.value();
    device = vkb_device.device;

    auto gfx_queue_ret = vkb_device.get_queue(vkb::QueueType::graphics);
    if(!gfx_queue_ret) {
        spdlog::error("Could not retrieve a graphics queue from logical device.");
        spdlog::error(gfx_queue_ret.error().message());
        return false;
    }
    gfx_queue = gfx_queue_ret.value();
    gfx_queue_index = vkb_device.get_queue_index(vkb::QueueType::graphics).value();

    auto present_queue_ret = vkb_device.get_queue(vkb::QueueType::present);
    if(!present_queue_ret) {
        spdlog::error("Could not retrieve a presentation queue from logical device.");
        spdlog::error(present_queue_ret.error().message());
        return false;
    }
    present_queue = present_queue_ret.value();
    present_queue_index = vkb_device.get_queue_index(vkb::QueueType::present).value();

    auto compute_queue_ret = vkb_device.get_queue(vkb::QueueType::compute);
    if(!compute_queue_ret) {
        spdlog::error("Could not retreive a compute queue from logical device.");
        spdlog::error(compute_queue_ret.error().message());
        return false;
    }
    compute_queue = compute_queue_ret.value();
    compute_queue_index = vkb_device.get_queue_index(vkb::QueueType::compute).value();

    auto transfer_queue_ret = vkb_device.get_dedicated_queue(vkb::QueueType::transfer);
    if(!transfer_queue_ret) {
        transfer_queue = gfx_queue;
        transfer_queue_index = gfx_queue_index;
    } else {
        transfer_queue = transfer_queue_ret.value();
        transfer_queue_index = vkb_device.get_dedicated_queue_index(vkb::QueueType::transfer).value();
    }
    
    uint32_t width;
    uint32_t height;
    glfwGetWindowSize(window, reinterpret_cast<int*>(&width), reinterpret_cast<int*>(&height));
    
    if(!init_vma(vkb_physDevice.physical_device, device, vkb_instance.instance)) return false;
    if(!init_swapchain(width, height)) return false;
    if(!init_commands()) return false;
    if(!init_renderpass()) return false;
    if(!init_framebuffers()) return false;
    if(!init_sync_structures()) return false;
    return true;
}

bool Engine::init_commands() {
    default_gfx_pool = new CommandPool(device, gfx_queue_index);
    default_comp_pool = new CommandPool(device, compute_queue_index);
    default_transfer_pool = new CommandPool(device, transfer_queue_index);
    cmd_buffers.insert({ default_gfx_pool, std::list<CommandBuffer*>() });
    cmd_buffers.insert({ default_comp_pool, std::list<CommandBuffer*>() });
    cmd_buffers.insert({ default_transfer_pool, std::list<CommandBuffer*>() });
    default_gfx_buffer = allocateBuffer(QueueType::GRAPHICS);
    return true;
}

bool Engine::init_swapchain(uint32_t width, uint32_t height) {
    SwapchainBuilder builder { vkb_physDevice.physical_device, device, surface,
                               gfx_queue_index, present_queue_index };
    auto sc_ret = builder.use_default_format_selection()
                         .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                         .set_desired_extent(width, height)
                         .build();
    if(!sc_ret) {
        spdlog::error("Failed to create a swapchain.");
        spdlog::error(sc_ret.error().message());
        return false;
    }
    vkb_swapchain = sc_ret.value();
    this->width = width;
    this->height = height;

    imageViews = vkb_swapchain.get_image_views().value();

    return true;
}

bool Engine::init_renderpass() {
    VkAttachmentDescription color_attach = {
        .format = vkb_swapchain.image_format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,

        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,

        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference attach_ref = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR
    };

    VkSubpassDescription subpass_desc = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attach_ref
    };

    VkRenderPassCreateInfo renderpass_info {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,

        .attachmentCount = 1,
        .pAttachments = &color_attach,

        .subpassCount = 1,
        .pSubpasses = &subpass_desc
    };

    if(vkCreateRenderPass(device, &renderpass_info, 0, &renderpass) != VK_SUCCESS) {
        spdlog::error("Could not create a render pass.");
        return false;
    }

    return true;
}

bool Engine::init_framebuffers() {
    VkFramebufferCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = renderpass,
        .attachmentCount = 1,
        .width = width,
        .height = height,
        .layers = 1
    };

    const int imgCount = imageViews.size();
    framebuffers.resize(imgCount);

    for(int i = 0; i < imgCount; i++) {
        createInfo.pAttachments = &imageViews.data()[i];
        if(vkCreateFramebuffer(device, &createInfo, 0, &framebuffers.data()[i]) != VK_SUCCESS) {
            spdlog::error("Failed to create a framebuffer.");
            return false;
        }
    }
    
    return true;
}

bool Engine::init_sync_structures() {
    VkFenceCreateInfo fenceInfo = { 
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };
    if(vkCreateFence(device, &fenceInfo, 0, &renderFence) != VK_SUCCESS) {
        spdlog::error("Failed to create a fence");
        return false;
    }

    VkSemaphoreCreateInfo semaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .flags = 0
    };

    if(vkCreateSemaphore(device, &semaphoreInfo, 0, &renderSemaphore) != VK_SUCCESS ||
       vkCreateSemaphore(device, &semaphoreInfo, 0, &presentSemaphore) != VK_SUCCESS) {
        spdlog::error("Failed to create a semaphore");
        return false;
    }
    return true;
}

void Engine::draw() {
    if(vkWaitForFences(device, 1, &renderFence, true, 0xFFFFFFFF) != VK_SUCCESS) return;
    if(vkResetFences(device, 1, &renderFence) != VK_SUCCESS) return;

    uint32_t imgIdx;
    if(vkAcquireNextImageKHR(device, vkb_swapchain.swapchain, 0xFFFFFFFF, presentSemaphore, 0, &imgIdx) != VK_SUCCESS) return;
    if(!default_gfx_buffer->resetBuffer()) return;
    if(!default_gfx_buffer->startRecording()) return;

    VkClearValue clearValue = { { 0.0f, 0.0f, 0.0f, 1.0f } };
    VkRenderPassBeginInfo rpbi = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = renderpass,
        .framebuffer = framebuffers.data()[imgIdx],
        .renderArea = {
            .offset = {
                .x = 0,
                .y = 0
            },
            .extent = {
                .width = width,
                .height = height
            }
        },
        .clearValueCount = 1,
        .pClearValues = &clearValue
    };

    vkCmdBeginRenderPass(default_gfx_buffer->getHandle(), &rpbi, VK_SUBPASS_CONTENTS_INLINE);

    for(int i = 0; i < renderCalls.size(); i++) renderCalls[i](default_gfx_buffer->getHandle());

    vkCmdEndRenderPass(default_gfx_buffer->getHandle());

    default_gfx_buffer->stopRecording();

    default_gfx_buffer->submit(gfx_queue, { presentSemaphore }, { renderSemaphore }, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, renderFence);

    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,

        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &renderSemaphore,

        .swapchainCount = 1,
        .pSwapchains = &vkb_swapchain.swapchain,

        .pImageIndices = &imgIdx
    };

    vkQueuePresentKHR(gfx_queue, &presentInfo);
}

VkDescriptorPool Engine::create_descriptor_pool() {
    VkDescriptorPoolSize poolSize[] = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 }
    };
    VkDescriptorPoolCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1000,
        .poolSizeCount = 1,
        .pPoolSizes = poolSize
    };

    VkDescriptorPool pool;
    if(vkCreateDescriptorPool(device, &createInfo, 0, &pool) != VK_SUCCESS) return 0;
    lastPool = pool;
    pools.push_back(pool);
    return pool;
}

VkDescriptorSet Engine::allocate_descriptor_set(VkDescriptorSetLayout layout) {
    std::lock_guard<std::recursive_mutex> lk(descriptorAllocatorLock);
    if(lastPool == 0) create_descriptor_pool();
    VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = 0,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout
    };

    VkDescriptorSet set;
    VkResult result;
    for(auto pool = pools.begin(); pool != pools.end(); pool++) {
        allocInfo.descriptorPool = *pool;
        result = vkAllocateDescriptorSets(device, &allocInfo, &set);
        switch(result) {
            case VK_SUCCESS: return set;
            case VK_ERROR_OUT_OF_POOL_MEMORY: break;
            default: return 0;
        }
    }
    create_descriptor_pool();
    return allocate_descriptor_set(layout);
}

void Engine::addRenderFunction(std::function<void(VkCommandBuffer)> renderFunc) {
    renderCalls.push_back(renderFunc);
}

size_t Engine::padUbo(size_t bufferSize) {
    uint32_t minAlignment = vkb_physDevice.properties.limits.minUniformBufferOffsetAlignment;
    size_t aligned = bufferSize;
    if(minAlignment > 0) aligned = (aligned + minAlignment - 1) & ~(minAlignment - 1);
    return aligned;
}

CommandBuffer* Engine::allocateBuffer(QueueType type) {
    switch(type) {
        case GRAPHICS: return allocateBuffer(default_gfx_pool);
        case COMPUTE: return allocateBuffer(default_comp_pool);
        case TRANSFER: return allocateBuffer(default_transfer_pool);
        default: return 0;
    }
}

CommandBuffer* Engine::allocateBuffer(CommandPool* pool) {
    CommandBuffer* buffer = new CommandBuffer(device, pool->getHandle());
    cmd_buffers[pool].push_back(buffer);
    cmd_buffer_pools.insert({ buffer, pool });
    return buffer;
}

CommandPool* Engine::allocatePool(QueueType type) {
    CommandPool* pool;
    switch(type) {
        case GRAPHICS: pool = new CommandPool(device, gfx_queue_index); break;
        case COMPUTE: pool = new CommandPool(device, compute_queue_index); break;
        case TRANSFER: pool = new CommandPool(device, transfer_queue_index); break;
        default: return 0;
    }
    cmd_buffers.insert({ pool, std::list<CommandBuffer*>() });
    return pool;
}

void Engine::freeBuffer(CommandBuffer* buffer) {
    CommandPool* pool = cmd_buffer_pools[buffer];
    cmd_buffers[pool].remove(buffer);

    bool queueDelete = false;
    if(cmd_buffers[pool].size() == 0) queueDelete = true;

    cmd_buffer_pools.erase(buffer);
    delete buffer;

    if(queueDelete) {
        if(pool == default_gfx_pool || pool == default_comp_pool) return;

        cmd_buffers.erase(pool);
        delete pool;
    }
}

VkQueue Engine::getQueue(QueueType type) {
    switch(type) {
        case GRAPHICS: return gfx_queue;
        case PRESENT: return present_queue;
        case COMPUTE: return compute_queue;
        case TRANSFER: return transfer_queue;
        default: return 0;
    }
}

void Engine::transfer(VkBuffer src, VkBuffer dst, size_t length) {
    CommandBuffer* buffer = allocateBuffer(TRANSFER);
    VkFence fence;
    VkFenceCreateInfo info = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT };
    if(vkCreateFence(device, &info, 0, &fence) != VK_SUCCESS) return;
    if(buffer->startRecording()) {
        VkBufferCopy region = {
            .srcOffset = 0,
            .dstOffset = 0,
            .size = length
        };
        vkCmdCopyBuffer(buffer->getHandle(), src, dst, 1, &region);
        buffer->stopRecording();

        buffer->submit(transfer_queue, { }, { }, 0, fence);
        vkWaitForFences(device, 1, &fence, VK_TRUE, 0xFFFFFFFF);
    }
    vkDestroyFence(device, fence, 0);
}
