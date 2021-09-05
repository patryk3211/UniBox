#include <compute/meshgen.hpp>

#include <simulator/voxel.hpp>
#include <simulator/particle.hpp>
#include <util/shader_assembler.hpp>
#include <util/finalizer.hpp>

#include <spdlog/spdlog.h>

#include <ostream>
#include <fstream>

using namespace unibox;

#define GROUP_SIZE_X 256
#define GROUP_SIZE_Y 1
#define GROUP_SIZE_Z 1

#define UBO_SIZE sizeof(float)*3
#define PARTICLE_INFO_PACKET_SIZE sizeof(float)*4

#define OUTPUT_VERTEX_SIZE sizeof(float)*4*2

cl::Buffer* MeshGenPipeline::pib = 0;
cl::Program* MeshGenPipeline::program = 0;

MeshGenPipeline::MeshGenPipeline() {
    
}

MeshGenPipeline::~MeshGenPipeline() {
    
}

void MeshGenPipeline::generate(uint32_t particleCount, cl::Buffer& particleBuffer, cl::Buffer& meshBuffer) {
    kernel.setArg(0, particleBuffer);
    kernel.setArg(1, meshBuffer);
    kernel.setArg(3, particleCount);

    cl_int error;
    cl::CommandQueue queue(ClEngine::getInstance()->getContext(), ClEngine::getInstance()->getDevice(), 0, &error);
    if(error != CL_SUCCESS) {
        spdlog::error("OpenCL Mesh Generator command queue creation failure: " + std::to_string(error));
        return;
    }
    error = queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(particleCount));
    if(error != CL_SUCCESS) {
        spdlog::error("OpenCL Mesh Gen Error: " + std::to_string(error));
        return;
    }

    queue.flush();

    error = queue.finish();
    if(error != CL_SUCCESS) {
        spdlog::error("OpenCL Mesh Gen Finish Error: " + std::to_string(error));
        return;
    }
}

void MeshGenPipeline::createMeshGenerationInformation() {
    if(pib == 0) {
        std::vector<Particle*>& particles = Particle::getParticleArray();

        ParticleInfoPacket pip[particles.size()];
        for(int i = 0; i < particles.size(); i++) particles[i]->fillPip(pip[i]);

        pib = new cl::Buffer(ClEngine::getInstance()->getContext(), CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS | CL_MEM_COPY_HOST_PTR, sizeof(ParticleInfoPacket)*particles.size(), pip);
        Finalizer::addCallback([](){ delete pib; });
    }
    kernel.setArg(2, *pib);
}

void MeshGenPipeline::createMeshGenerationShader() {
    if(program == 0) {
        ShaderAssembler assembler = ShaderAssembler("shaders/compute/meshGenerator.cl");
        assembler.pragmaInsert("PARTICLE_CODE", Particle::constructMeshFunctions());
        assembler.pragmaInsert("PARTICLE_SWITCH", Particle::constructMeshSwitchCode());
        assembler.pragmaInsert("PARTICLE_TYPES", Particle::constructTypeDefinitions());
        std::ofstream stream = std::ofstream("meshGenDump.cl", std::ios::binary);
        assembler.dump(stream);
        program = assembler.compile(ClEngine::getInstance()->getContext(), ClEngine::getInstance()->getDevice());
        Finalizer::addCallback([](){ delete program; });
    }

    kernel = cl::Kernel(*program, "generate");
}
