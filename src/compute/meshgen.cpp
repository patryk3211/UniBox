#include <compute/meshgen.hpp>

#include <vk-engine/engine.hpp>
#include <simulator/voxel.hpp>
#include <simulator/particle.hpp>

#include <spdlog/spdlog.h>

using namespace unibox;

#define GROUP_SIZE_X 256
#define GROUP_SIZE_Y 1
#define GROUP_SIZE_Z 1

#define UBO_SIZE sizeof(float)*3
#define PARTICLE_INFO_PACKET_SIZE sizeof(float)*4

#define OUTPUT_VERTEX_SIZE sizeof(float)*4*2

MeshGenPipeline::MeshGenPipeline() {
    this->device = Engine::getInstance()->getDevice();

    this->buffer = Engine::getInstance()->allocateBuffer(Engine::getInstance()->allocatePool(QueueType::COMPUTE));

    Shader shader = Shader(VK_SHADER_STAGE_COMPUTE_BIT, "main");
    shader.addCode("shaders/compute/meshGen.spv");

    this->pipeline = new ComputePipeline();
    this->pipeline->setShader(&shader);
    this->pipeline->addDescriptors(0, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT);
    this->pipeline->addDescriptors(0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT);
    this->pipeline->addDescriptors(0, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT);

    this->pipeline->addPushConstant(0, sizeof(uint), VK_SHADER_STAGE_COMPUTE_BIT);

    if(!this->pipeline->assemble()) {
        spdlog::error("Failed to initialize Mesh Generator compute pipeline.");
        return;
    }

    VkFenceCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };
    if(vkCreateFence(device, &info, 0, &fence) != VK_SUCCESS) {
        spdlog::error("Failed to create a sync fence in Mesh Generator.");
    }
}

MeshGenPipeline::~MeshGenPipeline() {
    delete this->pipeline;
    delete pib;
    Engine::getInstance()->freeBuffer(this->buffer);
    vkDestroyFence(device, fence, 0);
}

void MeshGenPipeline::generate(uint32_t particleCount, VkBuffer particleBuffer, VkBuffer meshBuffer) {
    if(vkWaitForFences(device, 1, &fence, VK_TRUE, 0xFFFFFFFF) != VK_SUCCESS) return;
    if(vkResetFences(device, 1, &fence) != VK_SUCCESS) return;

    if(!this->buffer->resetBuffer()) return;
    if(!this->buffer->startRecording()) return;
    VkCommandBuffer handle = this->buffer->getHandle();

    uint32_t groupCount = particleCount/GROUP_SIZE_X+(particleCount%GROUP_SIZE_X==0?0:1);

    struct {
        uint particleCount;
    } infoData;
    infoData.particleCount = particleCount;

    pipeline->bindBufferToDescriptor(0, 0, particleBuffer, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, sizeof(Voxel)*particleCount);
    pipeline->bindBufferToDescriptor(0, 1, meshBuffer, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, (OUTPUT_VERTEX_SIZE)*particleCount*6);

    vkCmdBindPipeline(handle, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->getHandle());
    vkCmdBindDescriptorSets(handle, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->getLayout(), 0, 1, pipeline->getDescriptorSet(), 0, 0);

    vkCmdPushConstants(handle, pipeline->getLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(infoData), &infoData);

    vkCmdDispatch(handle, groupCount, 1, 1);

    this->buffer->stopRecording();

    this->buffer->submit(Engine::getInstance()->getQueue(QueueType::COMPUTE), {}, {}, 0, fence);
}

bool MeshGenPipeline::isExecuting() {
    VkResult res = vkGetFenceStatus(device, fence);
    if(res != VK_NOT_READY && res != VK_SUCCESS) spdlog::error("Could not get status of Mesh Generator execution fence.");
    return res == VK_NOT_READY;
}

void MeshGenPipeline::createMeshGenerationInformation() {
    std::vector<Particle*>& particles = Particle::getParticleArray();

    pib = new Buffer((PARTICLE_INFO_PACKET_SIZE)*particles.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, VMA_MEMORY_USAGE_CPU_TO_GPU);
    this->pipeline->bindBufferToDescriptor(0, 2, pib->getHandle(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, (PARTICLE_INFO_PACKET_SIZE)*particles.size());

    void* ptr = pib->map();
    ParticleInfoPacket* pipPtr = (ParticleInfoPacket*)ptr;
    for(int i = 0; i < particles.size(); i++) particles[i]->fillPip(pipPtr[i]);
    pib->unmap();
}