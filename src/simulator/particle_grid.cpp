#include <simulator/particle_grid.hpp>

#include <spdlog/spdlog.h>
#include <glslang/Public/ShaderLang.h>
#include <vk-engine/engine.hpp>
#include <glm/mat4x4.hpp>
#include <util/finalizer.hpp>

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
    gridBuffer(ClEngine::getInstance()->getContext(), CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, width*height*length*sizeof(GridPoint)) {
    this->sizeX = width;
    this->sizeY = height;
    this->sizeZ = length;

    this->particleCount = 0;
    this->particleBuffer = new cl::Buffer(ClEngine::getInstance()->getContext(), CL_MEM_READ_WRITE, sizeof(Voxel)*256);
    cl::CommandQueue queue(ClEngine::getInstance()->getContext(), ClEngine::getInstance()->getDevice());
    particles = (Voxel*)queue.enqueueMapBuffer(*this->particleBuffer, CL_TRUE, CL_MAP_READ | CL_MAP_WRITE, 0, sizeof(Voxel)*256);
    
    for(int i = 0; i < 256; i++) freeIndices.push_back(i);

    meshBuffer = new cl::Buffer(ClEngine::getInstance()->getContext(), CL_MEM_READ_WRITE, (sizeof(float)*4*2)*6*256);
    meshBufferV = new Buffer((sizeof(float)*4*2)*6*256, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, VMA_MEMORY_USAGE_CPU_TO_GPU);

    dirty = true;

    grids.push_back(this);
}

ParticleGrid::~ParticleGrid() {
    grids.remove(this);
    cl::CommandQueue queue(ClEngine::getInstance()->getContext(), ClEngine::getInstance()->getDevice());
    queue.enqueueUnmapMemObject(*particleBuffer, particles);
    delete particleBuffer;
    delete meshBuffer;
    delete meshBufferV;
}

uint ParticleGrid::allocateParticleIndex() {
    if(freeIndices.size() == 0) {
        size_t base = particleCount;
        cl::Buffer* oldBuffer = this->particleBuffer;

        cl::CommandQueue queue(ClEngine::getInstance()->getContext(), ClEngine::getInstance()->getDevice());

        this->particleBuffer = new cl::Buffer(ClEngine::getInstance()->getContext(), CL_MEM_READ_WRITE | CL_MEM_KERNEL_READ_AND_WRITE, sizeof(Voxel)*(particleCount+256));
        queue.enqueueUnmapMemObject(*oldBuffer, this->particles);
        queue.enqueueCopyBuffer(*oldBuffer, *this->particleBuffer, 0, 0, sizeof(Voxel)*particleCount);
        this->particles = (Voxel*)queue.enqueueMapBuffer(*this->particleBuffer, CL_TRUE, CL_MAP_READ | CL_MAP_WRITE, 0, sizeof(Voxel)*(particleCount+256));

        delete oldBuffer;

        for(int i = 0; i < 256; i++) freeIndices.push_back(i+base);

        meshGenLock.lock();
        while(Engine::getInstance()->isRendering());
        delete this->meshBuffer;
        delete this->meshBufferV;
        meshGenLock.unlock();

        this->meshBuffer = new cl::Buffer(ClEngine::getInstance()->getContext(), CL_MEM_READ_WRITE, (sizeof(float)*4*2)*6*(particleCount+256));
        this->meshBufferV = new Buffer((sizeof(float)*4*2)*6*(particleCount+256), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, VMA_MEMORY_USAGE_CPU_TO_GPU);
    }
    uint index = *freeIndices.begin();
    freeIndices.pop_front();
    return index;
}

void ParticleGrid::addVoxel(const Voxel& voxel) {
    std::lock_guard lck(simLock);
    if(!isEmpty(static_cast<uint>(voxel.position[0]), static_cast<uint>(voxel.position[1]), static_cast<uint>(voxel.position[2]))) return;
    uint index = allocateParticleIndex();
    particles[index] = voxel;
    this->particleCount++;
    dirty = true;
}

void ParticleGrid::addVoxels(int x, int y, int z, const std::vector<Voxel>& voxels) {
    std::lock_guard lck(simLock);
    for(int i = 0; i < voxels.size(); i++) {
        Voxel v = voxels[i];
        v.position[0] += x;
        v.position[1] += y;
        v.position[2] += z;
        if(!isEmpty(static_cast<uint>(v.position[0]), static_cast<uint>(v.position[1]), static_cast<uint>(v.position[2]))) continue;
        particles[allocateParticleIndex()] = v;
        this->particleCount++;
    }
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
        meshGenerator->generate(particleCount, *particleBuffer, *meshBuffer);

        void* ptr = meshBufferV->map();
        cl::CommandQueue queue(ClEngine::getInstance()->getContext(), ClEngine::getInstance()->getDevice());
        queue.enqueueReadBuffer(*meshBuffer, CL_TRUE, 0, (sizeof(float)*4*2)*6*(particleCount), ptr);
        meshBufferV->unmap();

        dirty = false;
        meshGenLock.unlock();
    }

    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, &meshBufferV->getHandle(), offsets);
    vkCmdDraw(cmd, particleCount*6, 1, 0, 0);
}

void ParticleGrid::simulate() {
    if(particleCount == 0) return;
    std::lock_guard lck(simLock);
    cl::CommandQueue queue(ClEngine::getInstance()->getContext(), ClEngine::getInstance()->getDevice());
    queue.enqueueUnmapMemObject(*particleBuffer, particles);
    simulator->simulate(sizeX, sizeY, sizeZ, particleCount, gridBuffer, *particleBuffer);
    queue.enqueueMapBuffer(*particleBuffer, CL_TRUE, CL_MAP_READ | CL_MAP_WRITE, 0, sizeof(Voxel)*(particleCount/256+(particleCount%256==0?0:1)*256));
    dirty = true;
}

void ParticleGrid::init(Camera& camera) {
    if(!glslang::InitializeProcess()) {
        spdlog::error("Could not initialize glslang.");
        return;
    }
    Finalizer::addCallback([](){ glslang::FinalizeProcess(); });
    simInitSync = std::async(std::launch::async, [](){
        simulator = new Simulator();
        simulator->createSimulationShader();
        simulator->createSimulationInformation();
        Finalizer::addCallback([](){ delete simulator; });
    });
    meshInitSync = std::async(std::launch::async, [](){
        meshGenerator = new MeshGenPipeline();
        meshGenerator->createMeshGenerationShader();
        meshGenerator->createMeshGenerationInformation();
        Finalizer::addCallback([](){ delete meshGenerator; });
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
        Finalizer::addCallback([](){ delete pipeline; });
    });
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
