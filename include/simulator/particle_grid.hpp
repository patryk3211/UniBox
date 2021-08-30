#pragma once

#include <vector>
#include <future>
#include <optional>
#include <mutex>
#include <list>

#include <simulator/voxel.hpp>
#include <compute/simulator.hpp>
#include <compute/meshgen.hpp>
#include <vk-engine/buffer.hpp>
#include <vk-engine/gfxpipeline.hpp>
#include <renderer/camera.hpp>

namespace unibox {
    class ParticleGrid {
        static std::list<ParticleGrid*> grids;

        static Simulator* simulator;
        static MeshGenPipeline* meshGenerator;

        static GraphicsPipeline* pipeline;

        static std::mutex simLock;
        static std::mutex meshGenLock;

        Voxel* particles;
        std::list<uint> freeIndices;

        uint sizeX;
        uint sizeY;
        uint sizeZ;

        uint particleCount;

        Buffer* particleBuffer;
        Buffer* meshBuffer;
        Buffer gridBuffer;

        bool dirty;
    public:
        ParticleGrid(uint width, uint height, uint length);
        ~ParticleGrid();

        void addVoxel(const Voxel& voxel);
        void eraseVoxel(uint x, uint y, uint z);
        std::optional<Voxel*> getVoxel(uint x, uint y, uint z);
        bool isEmpty(uint x, uint y, uint z);

        void render(VkCommandBuffer cmd);
        void simulate();

        static void init(Camera& camera);
        static void finalize();
        static void waitInitComplete();

        static void renderAll(VkCommandBuffer cmd);
    };
}