#pragma once

#include <vk-engine/gfxpipeline.hpp>
#include <vulkan/vulkan.h>

#include <unordered_map>
#include <list>
#include <string>

namespace unibox {
    class Renderer {
        static std::unordered_map<std::string, GraphicsPipeline*> materials;
        static std::unordered_map<GraphicsPipeline*, std::list<Renderer*>> renderers;

        GraphicsPipeline* material;
    public:
        Renderer(const std::string& material);
        ~Renderer();

        virtual void render(VkCommandBuffer buffer);

        static void registerMaterial(const std::string& name, GraphicsPipeline* pipeline);
        static void renderAll(VkCommandBuffer buffer);
    };
}