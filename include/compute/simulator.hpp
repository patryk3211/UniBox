#pragma once

#include <cl-engine/engine.hpp>

namespace unibox {
    class Simulator {
        cl::Buffer pib;

        cl::Program* program;
        cl::Kernel simKernel;
        cl::Kernel resetKernel;
        cl::Kernel buildKernel;

    public:
        Simulator();
        ~Simulator();

        void simulate(uint32_t sizeX, uint32_t sizeY, uint32_t sizeZ, uint32_t particleCount, cl::Buffer gridBuffer, cl::Buffer particleBuffer);

        void createSimulationInformation();
        bool createSimulationShader();
    };
}
