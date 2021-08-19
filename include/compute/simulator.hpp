#pragma once

#include <vk-engine/computepipeline.hpp>
#include <vk-engine/commandbuffer.hpp>
#include <vk-engine/buffer.hpp>

namespace unibox {
    class Simulator {
        VkDevice device;

        ComputePipeline simulatePipeline;
        ComputePipeline buildPipeline;
        ComputePipeline resetPipeline;

        CommandBuffer* simCmd;
        CommandBuffer* buildCmd;
        CommandBuffer* resetCmd;

        Buffer* pib;

        VkFence simFence;
        VkFence resetFence;
        VkFence buildFence;

        VkSemaphore resetSemaphore;
        VkSemaphore buildSemaphore;

        bool recordSimBuffer(uint32_t sizeX, uint32_t sizeY, uint32_t sizeZ, uint32_t particleCount, VkBuffer gridBuffer, VkBuffer particleBuffer);
        bool recordBuildBuffer(uint32_t sizeX, uint32_t sizeY, uint32_t sizeZ, uint32_t particleCount, VkBuffer gridBuffer, VkBuffer particleBuffer);
        bool recordResetBuffer(uint32_t sizeX, uint32_t sizeY, uint32_t sizeZ, VkBuffer gridBuffer);
    public:
        Simulator();
        ~Simulator();

        void simulate(uint32_t sizeX, uint32_t sizeY, uint32_t sizeZ, uint32_t particleCount, VkBuffer gridBuffer, VkBuffer particleBuffer);
        bool isExecuting();

        void createSimulationInformation();
        bool createSimulationShader();
    };
}
