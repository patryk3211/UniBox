#include <simulator/particle_grid.hpp>

#include <spdlog/spdlog.h>
#include <glslang/Public/ShaderLang.h>
#include <vk-engine/engine.hpp>
#include <glm/mat4x4.hpp>

using namespace unibox;

std::list<ParticleGrid*> ParticleGrid::grids = std::list<ParticleGrid*>();

Simulator* ParticleGrid::simulator = 0;
MeshGenPipeline* ParticleGrid::meshGenerator = 0;

GraphicsPipeline* ParticleGrid::pipeline = 0;

std::mutex ParticleGrid::simLock = std::mutex();
std::mutex ParticleGrid::meshGenLock = std::mutex();

std::future<void> simInitSync;
std::future<void> meshInitSync;
std::future<void> pipelineCreatSync;

ParticleGrid::ParticleGrid(uint width, uint height, uint length) :
    gridBuffer(width*height*length*sizeof(GridPoint), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, VMA_MEMORY_USAGE_GPU_ONLY) {
    this->sizeX = width;
    this->sizeY = height;
    this->sizeZ = length;

    this->particleCount = 0;
    this->particleBuffer = new Buffer(sizeof(Voxel)*256, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, VMA_MEMORY_USAGE_CPU_TO_GPU);
    particles = reinterpret_cast<Voxel*>(this->particleBuffer->map());
    for(int i = 0; i < 256; i++) freeIndices.push_back(i);

    meshBuffer = new Buffer((sizeof(float)*4*2)*6*256, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, VMA_MEMORY_USAGE_GPU_ONLY);

    dirty = true;

    grids.push_back(this);
}

ParticleGrid::~ParticleGrid() {
    grids.remove(this);
    particleBuffer->unmap();
    delete particleBuffer;
    delete meshBuffer;
}

void ParticleGrid::addVoxel(const Voxel& voxel) {
    if(!isEmpty(static_cast<uint>(voxel.position[0]), static_cast<uint>(voxel.position[1]), static_cast<uint>(voxel.position[2]))) return;
    if(freeIndices.size() == 0) {
        size_t base = particleCount;
        Buffer* oldBuffer = this->particleBuffer;

        this->particleBuffer = new Buffer(sizeof(Voxel)*(particleCount+256), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, VMA_MEMORY_USAGE_CPU_TO_GPU); 
        Voxel* mapping = reinterpret_cast<Voxel*>(this->particleBuffer->map());
        memcpy(mapping, this->particles, particleCount*sizeof(Voxel));
        this->particles = mapping;

        oldBuffer->unmap();

        simLock.lock();
        while(simulator->isExecuting());
        delete oldBuffer;
        simLock.unlock();

        for(int i = 0; i < 256; i++) freeIndices.push_back(i+base);

        meshGenLock.lock();
        while(meshGenerator->isExecuting() || Engine::getInstance()->isRendering());
        delete this->meshBuffer;
        meshGenLock.unlock();

        this->meshBuffer = new Buffer((sizeof(float)*4*2)*6*(particleCount+256), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, VMA_MEMORY_USAGE_GPU_ONLY);
    }
    uint index = *freeIndices.begin();
    freeIndices.pop_front();
    particles[index] = voxel;
    this->particleCount++;
    dirty = true;
}

void ParticleGrid::eraseVoxel(uint x, uint y, uint z) {
    for(int i = 0; i < particleCount; i++) {
        if(static_cast<uint>(particles[i].position[0]) == x &&
           static_cast<uint>(particles[i].position[1]) == y &&
           static_cast<uint>(particles[i].position[2]) == z) {
            particles[i] = {};
            freeIndices.push_front(i);
            particleCount--;
            dirty = true;
            return;
        }
    }
}

std::optional<Voxel*> ParticleGrid::getVoxel(uint x, uint y, uint z) {
    for(int i = 0; i < particleCount; i++) {
        if(static_cast<uint>(particles[i].position[0]) == x &&
           static_cast<uint>(particles[i].position[1]) == y &&
           static_cast<uint>(particles[i].position[2]) == z) {
            return std::optional(&particles[i]);
        }
    }
    return std::nullopt;
}

bool ParticleGrid::isEmpty(uint x, uint y, uint z) {
    for(int i = 0; i < particleCount; i++) {
        if(static_cast<uint>(particles[i].position[0]) == x &&
           static_cast<uint>(particles[i].position[1]) == y &&
           static_cast<uint>(particles[i].position[2]) == z) {
            return false;
        }
    }
    return true;
}

void ParticleGrid::render(VkCommandBuffer cmd) {
    if(particleCount == 0) return;
    if(dirty) {
        meshGenLock.lock();
        meshGenerator->generate(particleCount, particleBuffer->getHandle(), meshBuffer->getHandle());
        meshGenLock.unlock();
    }

    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, &meshBuffer->getHandle(), offsets);
    vkCmdDraw(cmd, particleCount*6, 1, 0, 0);
}

void ParticleGrid::simulate() {
    if(particleCount == 0) return;
    simLock.lock();
    simulator->simulate(sizeX, sizeY, sizeZ, particleCount, gridBuffer.getHandle(), particleBuffer->getHandle());
    dirty = true;
    simLock.unlock();
}

void ParticleGrid::init(Camera& camera) {
    if(!glslang::InitializeProcess()) {
        spdlog::error("Could not initialize glslang.");
        return;
    }
    simInitSync = std::async(std::launch::async, [](){
        simulator = new Simulator();
        simulator->createSimulationShader();
        simulator->createSimulationInformation();
    });
    meshInitSync = std::async(std::launch::async, [](){
        meshGenerator = new MeshGenPipeline();
        meshGenerator->createMeshGenerationShader();
        meshGenerator->createMeshGenerationInformation();
    });
    pipelineCreatSync = std::async(std::launch::async, [&camera](){
        Shader vert = Shader(VK_SHADER_STAGE_VERTEX_BIT, "main");
        if(!vert.addCode("shaders/default/vertex.spv")) return;
        Shader frag = Shader(VK_SHADER_STAGE_FRAGMENT_BIT, "main");
        if(!frag.addCode("shaders/default/fragment.spv")) return;

        pipeline = new GraphicsPipeline();
        pipeline->addShader(&vert);
        pipeline->addShader(&frag);

        int bind = pipeline->addBinding(sizeof(float)*8, VK_VERTEX_INPUT_RATE_VERTEX);
        pipeline->addAttribute(bind, 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT);
        pipeline->addAttribute(bind, 1, sizeof(float)*4, VK_FORMAT_R32G32B32A32_SFLOAT);

        pipeline->addDescriptors(0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);

        if(!pipeline->assemble({ 1024, 720 }, [](VkDescriptorSetLayout layout) {
            return Engine::getInstance()->allocate_descriptor_set(layout);
        })) return;
        pipeline->bindBufferToDescriptor(0, 0, camera.getBuffer().getHandle(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, sizeof(glm::mat4)*2);
    });
}

void ParticleGrid::finalize() {
    delete simulator;
    delete meshGenerator;
    delete pipeline;
}

void ParticleGrid::waitInitComplete() {
    simInitSync.wait();
    meshInitSync.wait();
    pipelineCreatSync.wait();
}

void ParticleGrid::renderAll(VkCommandBuffer cmd) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getHandle());

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getLayout(), 0, 1, pipeline->getDescriptorSet(), 0, 0);
    for(auto& grid : grids) grid->render(cmd);
}
