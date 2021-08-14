#pragma once

#include <vk-engine/computepipeline.hpp>
#include <vk-engine/commandbuffer.hpp>
#include <vk-engine/buffer.hpp>

namespace unibox {
    class Simulator {
        VkDevice device;

        ComputePipeline simulatePipeline;
        ComputePipeline resetPipeline;

        CommandBuffer* cmd;

        Buffer ubo;

        VkFence fence;
    public:
        Simulator();
        ~Simulator();

        void simulate(uint32_t sizeX, uint32_t sizeY, uint32_t sizeZ, VkBuffer particleBuffer);
        bool isExecuting();
    };
}
