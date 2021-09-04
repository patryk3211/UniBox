#pragma once

#include <cl-engine/engine.hpp>

namespace unibox {
    class MeshGenPipeline {
        static cl::Buffer* pib; // Particle Info Buffer
        static cl::Program* program;

        cl::Kernel kernel;
    public:
        MeshGenPipeline();
        ~MeshGenPipeline();

        void generate(uint32_t particleCount, cl::Buffer& particleBuffer, cl::Buffer& meshBuffer);

        void createMeshGenerationInformation();
        void createMeshGenerationShader();
    };
}