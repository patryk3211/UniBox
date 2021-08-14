#pragma once
#include <renderer/renderer.hpp>

#include <vk-engine/buffer.hpp>
#include <vulkan/vulkan.h>

#include <string>

namespace unibox {
    class BufferRenderer : public Renderer {
        Buffer* buffer;

        size_t vertexCount;
    public:
        BufferRenderer(const std::string& material, size_t vertexCount, Buffer* buffer);
        ~BufferRenderer();

        void setVertexCount(size_t vertexCount);

        virtual void render(VkCommandBuffer buffer);
    };
}