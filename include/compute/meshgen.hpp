#pragma once

#include <vk-engine/computepipeline.hpp>
#include <vk-engine/commandbuffer.hpp>
#include <vk-engine/buffer.hpp>

namespace unibox {
    class MeshGenPipeline {
        VkDevice device;

        ComputePipeline* pipeline;
        CommandBuffer* buffer;

        Buffer* ubo;
        Buffer* pib; // Particle Info Buffer

        VkFence fence;
    public:
        MeshGenPipeline();
        ~MeshGenPipeline();

        void generate(uint32_t sizeX, uint32_t sizeY, uint32_t sizeZ, VkBuffer inputBuffer, VkBuffer meshBuffer);
        bool isExecuting();
    };
}