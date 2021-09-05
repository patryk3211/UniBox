#include <iostream>

#include <spdlog/spdlog.h>

#include <vk-engine/window.hpp>
#include <renderer/camera.hpp>
#include <util/savefile.hpp>
#include <simulator/particle_grid.hpp>
#include <cl-engine/engine.hpp>
#include <util/finalizer.hpp>

#include <chrono>
#include <future>
#include <cmath>

#include <glm/vec4.hpp>

using namespace unibox;

int main(int argc, char** argv) {
    spdlog::info("Welcome to UniBox!");

    Finalizer* finalizer = new Finalizer();

    ClEngine clEngine = ClEngine();

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
    zoom = 70.0f;
    camera->setPosition(glm::vec3(60, 65, 0));
    
    auto particleLoadSync = std::async(std::launch::async, Particle::loadParticles);

    particleLoadSync.wait();
    ParticleGrid::init(*camera);

    ParticleGrid::waitInitComplete();
    ParticleGrid* grid = new ParticleGrid(1024, 1024, 1);

    spdlog::info("Random gen start");
    {

        Voxel voxel = {};
        voxel.type = 1;
        voxel.data[0] = 0xFFFFFFFF;
        voxel.position[0] = 32;
        voxel.velocity[0] = 0;
        voxel.velocity[1] = 1;
        grid->addVoxel(voxel);

        voxel.type = 2;
        voxel.data[0] = 0;
        voxel.position[0] = 0;
        voxel.position[1] = 8;
        for(int i = 0; i < 64; i++) {
            voxel.position[0]++;
            grid->addVoxel(voxel);
        }

        /*voxel.data[1] = 0x00000000;
        voxel.data[0] = 0;

        for(int i = 0; i < 32; i++) {
            voxel.position[0]++;
            grid->addVoxel(voxel);
        }

        for(int i = 0; i < 32; i++) {
            voxel.position[1]++;
            grid->addVoxel(voxel);
        }

        for(int i = 0; i < 32; i++) {
            voxel.position[0]--;
            grid->addVoxel(voxel);
        }

        for(int i = 0; i < 30; i++) {
            voxel.position[1]--;
            grid->addVoxel(voxel);
        }

        voxel.position[1]--;
        voxel.data[0] = 1;
        grid->addVoxel(voxel);*/
        /*SaveFile file = SaveFile("saves/test.ubs");
        file.readParticles();
        grid->addVoxels(0, 0, 0, file.getParticles());*/
    }
    spdlog::info("Random gen end");

    window.getEngine().addRenderFunction(ParticleGrid::renderAll);

    auto last = std::chrono::high_resolution_clock::now();

    int f = 1;

    while(!window.shouldClose()) {
        window.frameStart();
        if(f == 0) {
            auto now = std::chrono::high_resolution_clock::now();
            auto dur = std::chrono::duration_cast<std::chrono::microseconds>(now-last);
            last=now;
            
            spdlog::info(std::to_string(1.0/(dur.count()/60000000.0)) + " FPS");
        }

        grid->simulate();

        glm::vec2 mouse = window.getCursorPos();
        mouse /= glm::vec2(1024/2.0, -720/2.0);
        mouse -= glm::vec2(1.0, -1.0);
        mouse *= glm::vec2(70*(1024.0/720.0), 70);
        mouse += glm::vec2(camera->getPosition().x, camera->getPosition().y);

        /*if(mouse.x > 0 && mouse.y > 0 && mouse.x < 1024 && mouse.y < 1024) {
            grid->addVoxel({
                .type = 1,
                .velocity = {
                    0,
                    0,
                    0
                },
                .position = {
                    mouse.x,
                    mouse.y,
                    0
                }
            });
        }*/

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

    delete camera;
    delete grid;

    delete finalizer;

    return 0;
}