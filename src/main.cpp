#include <iostream>

#include <spdlog/spdlog.h>

#include <vk-engine/window.hpp>
#include <vk-engine/gfxpipeline.hpp>
#include <renderer/buffer_renderer.hpp>
#include <renderer/camera.hpp>
#include <vk-engine/computepipeline.hpp>
#include <compute/meshgen.hpp>
#include <compute/simulator.hpp>
#include <simulator/particle.hpp>

#include <chrono>
#include <future>

#include <glm/vec4.hpp>

using namespace unibox;

ComputePipeline* compute_pipeline;
Buffer* cbuffer;
bool done = false;

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

    cbuffer = new Buffer(sizeof(Voxel)*256*64*64, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, VMA_MEMORY_USAGE_CPU_TO_GPU);
    Buffer* meshBuffer = new Buffer((sizeof(float)*4*2)*256*6*64*64, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, VMA_MEMORY_USAGE_GPU_ONLY);

    spdlog::info("Random gen start");
    void* buf = cbuffer->map();
    Voxel* voxels = (Voxel*)buf;
    for(int i = 0; i < 256*64*64; i++) {
        voxels[i].type = std::rand()%10 == 0 ? 1 : 0;
        //voxels[i].paintColor = glm::vec4(1.0, 1.0, 0.0, 0.5);
        /*voxels[i].color[0] = 1;
        voxels[i].color[1] = 1;
        voxels[i].color[2] = 1;
        voxels[i].color[3] = 1;*/
    }
    cbuffer->unmap();
    spdlog::info("Random gen end");

    particleInfoSync.wait();
    mg->generate(16*64, 16*64, 1, cbuffer->getHandle(), meshBuffer->getHandle());

    Simulator* sim = new Simulator();
    //sim->simulate(16*64, 16*64, 1, cbuffer->getHandle());

    BufferRenderer render = BufferRenderer("default", 256*6*64*64, meshBuffer);

    window.getEngine().addRenderFunction(Renderer::renderAll);

    //auto last = std::chrono::high_resolution_clock::now();

    int f = 0;

    while(!window.shouldClose()) {
        window.frameStart();
        /*auto now = std::chrono::high_resolution_clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::microseconds>(now-last);
        last=now;
        spdlog::info(dur.count());*/
        mg->generate(16*64, 16*64, 1, cbuffer->getHandle(), meshBuffer->getHandle());
        /*if(f == 0)*/ sim->simulate(16*64, 16*64, 1, cbuffer->getHandle());

        zoom += dir;
        if(zoom < 10.0f) dir = -dir;
        if(zoom > 600.0f) dir = -dir;

        camera->orthographic(1024.0f/720.0f, -10.0f, 10.0f, zoom);
        camera->updateBuffer();

        window.render();
        f++;
        f = f%30;
    }

    window.waitIdle();

    delete cbuffer;
    delete meshBuffer;
    delete mg;
    delete sim;
    delete default_pipeline;
    delete compute_pipeline;
    delete camera;

    return 0;
}