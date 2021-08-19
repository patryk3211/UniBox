#include <compute/simulator.hpp>

#include <vk-engine/engine.hpp>
#include <simulator/voxel.hpp>
#include <vk-engine/shader_assembler.hpp>
#include <simulator/particle.hpp>

#include <spdlog/spdlog.h>

#include <iostream>
#include <fstream>

using namespace unibox;

#define GROUP_SIZE_X 256

#define UBO_SIZE sizeof(uint)*3

#define PARTICLE_INFO_PACKET_SIZE sizeof(SimulationParticleInfoPacket)

Simulator::Simulator() {
    device = Engine::getInstance()->getDevice();

    CommandPool* pool = Engine::getInstance()->allocatePool(QueueType::COMPUTE);

    {
        simulatePipeline.addDescriptors(0, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT);
        simulatePipeline.addDescriptors(0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT);
        simulatePipeline.addDescriptors(0, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT);

        simulatePipeline.addPushConstant(0, sizeof(uint)*5, VK_SHADER_STAGE_COMPUTE_BIT);

        simCmd = Engine::getInstance()->allocateBuffer(pool);
    }
    {
        Shader shader = Shader(VK_SHADER_STAGE_COMPUTE_BIT, "main");
        shader.addCode("shaders/compute/gridBuilder.spv");

        buildPipeline.setShader(&shader);
        buildPipeline.addDescriptors(0, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT);
        buildPipeline.addDescriptors(0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT);

        buildPipeline.addPushConstant(0, sizeof(uint)*4, VK_SHADER_STAGE_COMPUTE_BIT);

        if(!buildPipeline.assemble()) {
            spdlog::error("Could not assemble the Grid Build Pipeline.");
            return;
        }

        buildCmd = Engine::getInstance()->allocateBuffer(pool);
    }
    {
        Shader shader = Shader(VK_SHADER_STAGE_COMPUTE_BIT, "main");
        shader.addCode("shaders/compute/gridResetter.spv");

        resetPipeline.setShader(&shader);
        resetPipeline.addDescriptors(0, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT);

        resetPipeline.addPushConstant(0, sizeof(uint)*3, VK_SHADER_STAGE_COMPUTE_BIT);

        if(!resetPipeline.assemble()) {
            spdlog::error("Could not assemble the Grid Reset Pipeline.");
            return;
        }

        resetCmd = Engine::getInstance()->allocateBuffer(pool);
    }

    {
        VkFenceCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };

        if(vkCreateFence(device, &info, 0, &simFence) != VK_SUCCESS ||
           vkCreateFence(device, &info, 0, &resetFence) != VK_SUCCESS ||
           vkCreateFence(device, &info, 0, &buildFence) != VK_SUCCESS) {
            spdlog::error("Could not create a synchronization fence in Simulator.");
            return;
        }
    }

    {
        VkSemaphoreCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
        };

        if(vkCreateSemaphore(device, &info, 0, &resetSemaphore) != VK_SUCCESS ||
           vkCreateSemaphore(device, &info, 0, &buildSemaphore) != VK_SUCCESS) {
            spdlog::error("Could not create a synchronization semaphore in Simulator.");
            return;
        }
    }

    pib = 0;
}

Simulator::~Simulator() {
    Engine::getInstance()->freeBuffer(simCmd);
    Engine::getInstance()->freeBuffer(buildCmd);
    Engine::getInstance()->freeBuffer(resetCmd);
    vkDestroyFence(device, simFence, 0);
    vkDestroyFence(device, resetFence, 0);
    vkDestroyFence(device, buildFence, 0);
    vkDestroySemaphore(device, resetSemaphore, 0);
    vkDestroySemaphore(device, buildSemaphore, 0);
    delete pib;
}

bool Simulator::recordSimBuffer(uint32_t sizeX, uint32_t sizeY, uint32_t sizeZ, uint32_t particleCount, VkBuffer gridBuffer, VkBuffer particleBuffer) {
    if(vkWaitForFences(device, 1, &simFence, true, 0xFFFFFFFF) != VK_SUCCESS) return false;
    if(vkResetFences(device, 1, &simFence) != VK_SUCCESS) return false;

    if(!simCmd->resetBuffer()) return false;
    if(!simCmd->startRecording()) return false;
    VkCommandBuffer handle = simCmd->getHandle();

    uint32_t count = sizeX*sizeY*sizeZ;
    uint32_t groupCount = particleCount/GROUP_SIZE_X+(particleCount%GROUP_SIZE_X==0?0:1);

    struct {
        uint sizeX;
        uint sizeY;
        uint sizeZ;
        uint particleCount;
        uint updatesPerSecond;
    } infoData;
    infoData.sizeX = sizeX;
    infoData.sizeY = sizeY;
    infoData.sizeZ = sizeZ;
    infoData.particleCount = particleCount;
    infoData.updatesPerSecond = 60;

    simulatePipeline.bindBufferToDescriptor(0, 0, gridBuffer, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, sizeof(GridPoint)*count);
    simulatePipeline.bindBufferToDescriptor(0, 1, particleBuffer, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, sizeof(Voxel)*particleCount);

    vkCmdBindPipeline(handle, VK_PIPELINE_BIND_POINT_COMPUTE, simulatePipeline.getHandle());
    vkCmdBindDescriptorSets(handle, VK_PIPELINE_BIND_POINT_COMPUTE, simulatePipeline.getLayout(), 0, 1, simulatePipeline.getDescriptorSet(), 0, 0);

    vkCmdPushConstants(handle, simulatePipeline.getLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(infoData), &infoData);

    vkCmdDispatch(handle, groupCount, 1, 1);

    simCmd->stopRecording();

    return true;
}

bool Simulator::recordBuildBuffer(uint32_t sizeX, uint32_t sizeY, uint32_t sizeZ, uint32_t particleCount, VkBuffer gridBuffer, VkBuffer particleBuffer) {
    if(vkWaitForFences(device, 1, &buildFence, true, 0xFFFFFFFF) != VK_SUCCESS) return false;
    if(vkResetFences(device, 1, &buildFence) != VK_SUCCESS) return false;

    if(!buildCmd->resetBuffer()) return false;
    if(!buildCmd->startRecording()) return false;
    VkCommandBuffer handle = buildCmd->getHandle();

    uint32_t count = sizeX*sizeY*sizeZ;
    uint32_t groupCount = particleCount/GROUP_SIZE_X+(particleCount%GROUP_SIZE_X==0?0:1);

    struct {
        uint sizeX;
        uint sizeY;
        uint sizeZ;
        uint particleCount;
    } infoData;
    infoData.sizeX = sizeX;
    infoData.sizeY = sizeY;
    infoData.sizeZ = sizeZ;
    infoData.particleCount = particleCount;

    buildPipeline.bindBufferToDescriptor(0, 0, gridBuffer, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, sizeof(GridPoint)*count);
    buildPipeline.bindBufferToDescriptor(0, 1, particleBuffer, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, sizeof(Voxel)*particleCount);

    vkCmdBindPipeline(handle, VK_PIPELINE_BIND_POINT_COMPUTE, buildPipeline.getHandle());
    vkCmdBindDescriptorSets(handle, VK_PIPELINE_BIND_POINT_COMPUTE, buildPipeline.getLayout(), 0, 1, buildPipeline.getDescriptorSet(), 0, 0);

    vkCmdPushConstants(handle, buildPipeline.getLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(infoData), &infoData);

    vkCmdDispatch(handle, groupCount, 1, 1);

    buildCmd->stopRecording();

    return true;
}

bool Simulator::recordResetBuffer(uint32_t sizeX, uint32_t sizeY, uint32_t sizeZ, VkBuffer gridBuffer) {
    if(vkWaitForFences(device, 1, &resetFence, true, 0xFFFFFFFF) != VK_SUCCESS) return false;
    if(vkResetFences(device, 1, &resetFence) != VK_SUCCESS) return false;

    if(!resetCmd->resetBuffer()) return false;
    if(!resetCmd->startRecording()) return false;
    VkCommandBuffer handle = resetCmd->getHandle();

    uint32_t count = sizeX*sizeY*sizeZ;
    uint32_t groupCount = count/GROUP_SIZE_X+(count%GROUP_SIZE_X==0?0:1);

    struct {
        uint sizeX;
        uint sizeY;
        uint sizeZ;
    } infoData;
    infoData.sizeX = sizeX;
    infoData.sizeY = sizeY;
    infoData.sizeZ = sizeZ;

    resetPipeline.bindBufferToDescriptor(0, 0, gridBuffer, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, sizeof(GridPoint)*count);

    vkCmdBindPipeline(handle, VK_PIPELINE_BIND_POINT_COMPUTE, resetPipeline.getHandle());
    vkCmdBindDescriptorSets(handle, VK_PIPELINE_BIND_POINT_COMPUTE, resetPipeline.getLayout(), 0, 1, resetPipeline.getDescriptorSet(), 0, 0);

    vkCmdPushConstants(handle, resetPipeline.getLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(infoData), &infoData);

    vkCmdDispatch(handle, groupCount, 1, 1);

    resetCmd->stopRecording();

    return true;
}

void Simulator::simulate(uint32_t sizeX, uint32_t sizeY, uint32_t sizeZ, uint32_t particleCount, VkBuffer gridBuffer, VkBuffer particleBuffer) {
    if(!recordResetBuffer(sizeX, sizeY, sizeZ, gridBuffer)) return;
    VkQueue queue = Engine::getInstance()->getQueue(QueueType::COMPUTE);
    resetCmd->submit(queue, {}, { resetSemaphore }, 0, resetFence);

    if(!recordBuildBuffer(sizeX, sizeY, sizeZ, particleCount, gridBuffer, particleBuffer)) return;
    buildCmd->submit(queue, { resetSemaphore }, { buildSemaphore }, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, buildFence);

    if(!recordSimBuffer(sizeX, sizeY, sizeZ, particleCount, gridBuffer, particleBuffer)) return;
    simCmd->submit(Engine::getInstance()->getQueue(QueueType::COMPUTE), { buildSemaphore }, {}, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, simFence);
}

bool Simulator::isExecuting() {
    VkResult res = vkGetFenceStatus(device, simFence);
    if(res != VK_NOT_READY && res != VK_SUCCESS) spdlog::error("Could not get status of Mesh Generator execution fence.");
    return res == VK_NOT_READY;
}

void Simulator::createSimulationInformation() {
    std::vector<Particle*>& particles = Particle::getParticleArray();

    pib = new Buffer((PARTICLE_INFO_PACKET_SIZE)*particles.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, VMA_MEMORY_USAGE_CPU_TO_GPU);
    this->simulatePipeline.bindBufferToDescriptor(0, 2, pib->getHandle(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, (PARTICLE_INFO_PACKET_SIZE)*particles.size());

    void* ptr = pib->map();
    SimulationParticleInfoPacket* pipPtr = (SimulationParticleInfoPacket*)ptr;
    for(int i = 0; i < particles.size(); i++) particles[i]->fillSimPip(pipPtr[i]);
    pib->unmap();
}

bool Simulator::createSimulationShader() {
    Shader shader = Shader(VK_SHADER_STAGE_COMPUTE_BIT, "main");
    ShaderAssembler assembler = ShaderAssembler("shaders/compute/simulator.comp");
    assembler.pragmaInsert("PARTICLE_CODE", Particle::constructFunctions());
    assembler.pragmaInsert("PARTICLE_SWITCH", Particle::constructSwitchCode());
    std::vector<uint32_t>& code = assembler.compile(EShLanguage::EShLangCompute);
    std::ofstream stream = std::ofstream("dump.comp", std::ios::binary);
    assembler.dump(stream);

    shader.addCode(code.size()*4, code.data());
    simulatePipeline.setShader(&shader);

    if(!simulatePipeline.assemble()) {
        spdlog::error("Could not assemble the Simulation Pipeline.");
        return false;
    }
    return true;
}
