#include <compute/simulator.hpp>

#include <simulator/voxel.hpp>
#include <util/shader_assembler.hpp>
#include <simulator/particle.hpp>

#include <spdlog/spdlog.h>

#include <iostream>
#include <fstream>

using namespace unibox;

#define GROUP_SIZE_X 256

#define UBO_SIZE sizeof(uint)*3

#define PARTICLE_INFO_PACKET_SIZE sizeof(SimulationParticleInfoPacket)

Simulator::Simulator() {
    
}

Simulator::~Simulator() {
    if(program != 0) delete program;
}

void Simulator::simulate(uint32_t sizeX, uint32_t sizeY, uint32_t sizeZ, uint32_t particleCount, cl::Buffer gridBuffer, cl::Buffer particleBuffer) {
    cl_int error;
    cl::CommandQueue queue = cl::CommandQueue(ClEngine::getInstance()->getContext(), ClEngine::getInstance()->getDevice(), 0, &error);
    if(error != CL_SUCCESS) {
        spdlog::error("OpenCL Simulator Command Queue creation failure: " + std::to_string(error));
        return;
    }

    resetKernel.setArg(0, gridBuffer);
    resetKernel.setArg(1, sizeX);
    resetKernel.setArg(2, sizeY);
    resetKernel.setArg(3, sizeZ);
    std::vector<cl::Event> resetEvent = { cl::Event() };
    error = queue.enqueueNDRangeKernel(resetKernel, cl::NullRange, cl::NDRange(sizeX*sizeY*sizeZ), cl::NullRange, 0, &resetEvent[0]);
    if(error != CL_SUCCESS) {
        spdlog::error("OpenCL Grid Reset Error: " + std::to_string(error));
        return;
    }

    buildKernel.setArg(0, particleBuffer);
    buildKernel.setArg(1, gridBuffer);
    buildKernel.setArg(2, sizeX);
    buildKernel.setArg(3, sizeY);
    buildKernel.setArg(4, sizeZ);
    buildKernel.setArg(5, particleCount);
    std::vector<cl::Event> buildEvent = { cl::Event() };
    error = queue.enqueueNDRangeKernel(buildKernel, cl::NullRange, cl::NDRange(particleCount), cl::NullRange, &resetEvent, &buildEvent[0]);
    if(error != CL_SUCCESS) {
        spdlog::error("OpenCL Grid Build Error: " + std::to_string(error));
        return;
    }

    simKernel.setArg(0, particleBuffer);
    simKernel.setArg(1, gridBuffer);
    simKernel.setArg(3, sizeX);
    simKernel.setArg(4, sizeY);
    simKernel.setArg(5, sizeZ);
    simKernel.setArg(6, particleCount);
    simKernel.setArg(7, 60);
    error = queue.enqueueNDRangeKernel(simKernel, cl::NullRange, cl::NDRange(particleCount), cl::NullRange, &buildEvent, 0);
    if(error != CL_SUCCESS) {
        spdlog::error("OpenCL Simulate Error: " + std::to_string(error));
        return;
    }

    error = queue.finish();
    if(error != CL_SUCCESS) {
        spdlog::error("OpenCL Simulator finish error: " + std::to_string(error));
        return;
    }
}

void Simulator::createSimulationInformation() {
    std::vector<Particle*>& particles = Particle::getParticleArray();

    SimulationParticleInfoPacket pipPtr[particles.size()];
    for(int i = 0; i < particles.size(); i++) particles[i]->fillSimPip(pipPtr[i]);

    pib = cl::Buffer(ClEngine::getInstance()->getContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS, sizeof(SimulationParticleInfoPacket)*particles.size(), pipPtr);
    simKernel.setArg(2, pib);
}

bool Simulator::createSimulationShader() {
    ShaderAssembler assembler = ShaderAssembler("shaders/compute/simulator.cl");
    assembler.pragmaInsert("PARTICLE_CODE", Particle::constructFunctions());
    while(assembler.hasPragma("COMPILATION_REMOVE")) assembler.pragmaRemove("COMPILATION_REMOVE", "END_COMPILATION_REMOVE");
    assembler.pragmaInsert("PARTICLE_SWITCH", Particle::constructSwitchCode());
    assembler.pragmaInsert("PARTICLE_TYPES", Particle::constructTypeDefinitions());
    std::ofstream stream = std::ofstream("simDump.cl", std::ios::binary);
    assembler.dump(stream);

    program = assembler.compile(ClEngine::getInstance()->getContext(), ClEngine::getInstance()->getDevice());
    if(program == 0) return false;

    simKernel = cl::Kernel(*program, "simulate");
    resetKernel = cl::Kernel(*program, "resetGrid");
    buildKernel = cl::Kernel(*program, "buildGrid");
    return true;
}
