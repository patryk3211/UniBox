#include <iostream>

#include <spdlog/spdlog.h>
#include <glslang/Public/ShaderLang.h>

#include <vk-engine/window.hpp>
#include <vk-engine/gfxpipeline.hpp>
#include <renderer/buffer_renderer.hpp>
#include <renderer/camera.hpp>
#include <vk-engine/computepipeline.hpp>
#include <compute/meshgen.hpp>
#include <compute/simulator.hpp>
#include <simulator/particle.hpp>
#include <util/savefile.hpp>

#include <chrono>
#include <future>

#include <glm/vec4.hpp>

using namespace unibox;

int main(int argc, char** argv) {
    spdlog::info("Welcome to UniBox!");

    Window window = Window();
    if(!window.init()) return -1;
    spdlog::info("Window created succesfully.");

    float zoom = 600.0f;
    float dir = 0;//-0.1f;

    Camera* camera = new Camera();
    camera->setPosition(glm::vec3(450.0f, 450.0f, 1.0f));
    camera->setRotation(glm::vec3(0.0f, 0.0f, 0.0f));
    camera->orthographic(1024.0f/720.0f, -10.0f, 10.0f, zoom);
    //camera->perspective(90.0f, 1024.0f/720.0f);
    //zoom = 70.0f;
    //camera->setPosition(glm::vec3(60, 65, 0));

    GraphicsPipeline* default_pipeline;
    {
        Shader vert = Shader(VK_SHADER_STAGE_VERTEX_BIT, "main");
        if(!vert.addCode("shaders/default/vertex.spv")) return false;
        Shader frag = Shader(VK_SHADER_STAGE_FRAGMENT_BIT, "main");
        if(!frag.addCode("shaders/default/fragment.spv")) return false;

        default_pipeline = new GraphicsPipeline();
        default_pipeline->addShader(&vert);
        default_pipeline->addShader(&frag);

        int bind = default_pipeline->addBinding(sizeof(float)*8, VK_VERTEX_INPUT_RATE_VERTEX);
        default_pipeline->addAttribute(bind, 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT);
        default_pipeline->addAttribute(bind, 1, sizeof(float)*4, VK_FORMAT_R32G32B32A32_SFLOAT);

        default_pipeline->addDescriptors(0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);

        if(!default_pipeline->assemble({ 1024, 720 }, [](VkDescriptorSetLayout layout) {
            return Engine::getInstance()->allocate_descriptor_set(layout);
        })) return false;
        default_pipeline->bindBufferToDescriptor(0, 0, camera->getBuffer().getHandle(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, sizeof(glm::mat4)*2);

        Renderer::registerMaterial("default", default_pipeline);
    }
    
    auto particleLoadSync = std::async(std::launch::async, Particle::loadParticles);

    MeshGenPipeline* mg = new MeshGenPipeline();
    particleLoadSync.wait();
    auto particleInfoSync = std::async(std::launch::async, [mg](){ mg->createMeshGenerationInformation(); });

    if(!glslang::InitializeProcess()) {
        spdlog::error("Could not initialize glslang.");
        return -1;
    }
    Simulator* sim = new Simulator();
    auto simInfoSync = std::async(std::launch::async, [sim](){
        sim->createSimulationShader();
        sim->createSimulationInformation();
    });

    Buffer* gridBuffer = new Buffer(sizeof(GridPoint)*256*64*64, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, VMA_MEMORY_USAGE_CPU_TO_GPU);
    Buffer* particleBuffer;
    
    Buffer* meshBuffer = new Buffer((sizeof(float)*4*2)*256*6*64*64, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, VMA_MEMORY_USAGE_GPU_ONLY);

    spdlog::info("Random gen start");
    uint particleCount = 0;
    {
        /*SaveFile file = SaveFile("saves/test.ubs");
        file.readParticles();*/
        std::vector<Voxel> particles;// = file.getParticles();

        for(int i = 0; i < 100000; i++) {
            Voxel voxel = {};
            voxel.type = 2;
            voxel.position[0] = std::rand()%1024;
            voxel.position[1] = std::rand()%1024;
            voxel.velocity[0] = -1;
            voxel.velocity[1] = -1;
            //voxel.velocity[0] = -1;//(float)(std::rand()%10)/10.0f;
            //voxel.velocity[1] = 0;//(float)(std::rand()%10)/10.0f;
            //voxel.paintColor[3] = 0.5;
            //voxel.paintColor[0] = 1;
            particles.push_back(voxel);
        }

        particleCount = particles.size();
        particleBuffer = new Buffer(sizeof(Voxel)*particleCount, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, VMA_MEMORY_USAGE_CPU_TO_GPU);
        particleBuffer->store(particles.data(), 0, sizeof(Voxel)*particleCount);
    }
    spdlog::info("Random gen end");

    uint size = 16*64;

    particleInfoSync.wait();

    mg->generate(particleCount, particleBuffer->getHandle(), meshBuffer->getHandle());
    BufferRenderer render = BufferRenderer("default", particleCount*6, meshBuffer);
    window.getEngine().addRenderFunction(Renderer::renderAll);

    //auto last = std::chrono::high_resolution_clock::now();

    int f = 1;

    simInfoSync.wait();
    while(!window.shouldClose()) {
        window.frameStart();
        /*auto now = std::chrono::high_resolution_clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::microseconds>(now-last);
        last=now;
        spdlog::info(dur.count());*/

        sim->simulate(size, size, 1, particleCount, gridBuffer->getHandle(), particleBuffer->getHandle());
        mg->generate(particleCount, particleBuffer->getHandle(), meshBuffer->getHandle());

        /*Voxel* particles = (Voxel*)particleBuffer->map();
        spdlog::info(particles[0].position[0]);
        particleBuffer->unmap();*/

        zoom += dir;
        if(zoom < 10.0f) dir = -dir;
        if(zoom > 600.0f) dir = -dir;

        camera->orthographic(1024.0f/720.0f, -10.0f, 10.0f, zoom);
        camera->updateBuffer();

        window.render();
        f++;
        f = f%60;
    }

    window.waitIdle();

    delete gridBuffer;
    delete particleBuffer;
    delete meshBuffer;
    delete mg;
    delete sim;
    delete default_pipeline;
    delete camera;

    glslang::FinalizeProcess();

    return 0;
}