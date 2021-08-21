#pragma once

#include <vk-engine/computepipeline.hpp>
#include <vk-engine/commandbuffer.hpp>
#include <vk-engine/buffer.hpp>

namespace unibox {
    class MeshGenPipeline {
        VkDevice device;

        ComputePipeline pipeline;
        CommandBuffer* buffer;

        Buffer* pib; // Particle Info Buffer

        VkFence fence;
    public:
        MeshGenPipeline();
        ~MeshGenPipeline();

        void generate(uint32_t particleCount, VkBuffer particleBuffer, VkBuffer meshBuffer);
        bool isExecuting();

        void createMeshGenerationInformation();
        void createMeshGenerationShader();
    };
}