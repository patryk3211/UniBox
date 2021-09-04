typedef struct {
    ushort type;
    ushort stype;
    float paintColor[4];
    int data[4];
    float temperature;
    float velocity[3];
    float position[3];
    uint state; // bits 0-23 for use by particle, 24-31 reserved
    // There will be two shader runs, the first one will determine how much particles will be placed to allocate memory for them.
    // The second run will be the simulation run which will do all of the simulation and place the particles.
    // There will be a variable used in those two runs for different purposes, in the pre-sim run it acts as a newParticleCount variable,
    // in the second run it acts as the first id in particles array that can be used to store the newly created particles.
} Particle;

typedef struct {
    uint particleOffset;
} GridPoint;

typedef struct {
    float density;
    uint state; /**
                 * bits 0-1 = State Bits (00b - Solid, 01b - Liquid, 10b - Gas, 11b - Powder)
                 * bit 2 = Affected By Gravity (0b - false, 1b - true)
                 **/
} ParticleInfo;

typedef struct {
    global Particle* particles;
    global GridPoint* grid;
    constant ParticleInfo* particleInfo;

    uint sizeX;
    uint sizeY;
    uint sizeZ;
    uint particleCount;
} SimulationStructures;

// Utility functions
uint calculatePosition(const SimulationStructures simStruct, uint x, uint y, uint z);
bool checkBounds(const SimulationStructures simStruct, int x, int y, int z);
uint getOffset(const SimulationStructures simStruct, uint x, uint y, uint z);
bool isEmpty(const SimulationStructures simStruct, uint x, uint y, uint z);
Particle getParticle(const SimulationStructures simStruct, uint x, uint y, uint z);
ParticleInfo getParticleInfo(const SimulationStructures simStruct, Particle particle);

#pragma PARTICLE_TYPES

#pragma PARTICLE_CODE

__kernel void simulate(global Particle* particles, global GridPoint* grid, constant ParticleInfo* particleInfo, uint sizeX, uint sizeY, uint sizeZ, uint particleCount, uint updatesPerSecond) {
    uint index = get_global_id(0);
    if(index >= particleCount) return;

    SimulationStructures structs;
    structs.particles = particles;
    structs.grid = grid;
    structs.particleInfo = particleInfo;
    structs.sizeX = sizeX;
    structs.sizeY = sizeY;
    structs.sizeZ = sizeZ;
    structs.particleCount = particleCount;

    Particle particle = particles[index];

    switch(particle.type) {
        #pragma PARTICLE_SWITCH
        default: break;
    }

    particles[index] = particle;
}

__kernel void resetGrid(global GridPoint* grid, uint sizeX, uint sizeY, uint sizeZ) {
    uint index = get_global_id(0);
    if(index >= sizeX*sizeY*sizeZ) return;

    grid[index].particleOffset = 0;
}

__kernel void buildGrid(global Particle* particles, global GridPoint* grid, uint sizeX, uint sizeY, uint sizeZ, uint particleCount) {
    uint index = get_global_id(0);
    if(index >= particleCount) return;

    Particle particle = particles[index];
    if(particle.type == 0) return;

    uint vertexGridOffset = (uint)(particle.position[0])+(uint)(particle.position[1])*sizeX+(uint)(particle.position[2])*sizeX*sizeY;
    grid[vertexGridOffset].particleOffset = index+1;
}

// Utility functions
uint calculatePosition(const SimulationStructures simStruct, uint x, uint y, uint z) {
    return x + y*simStruct.sizeX + z*simStruct.sizeX*simStruct.sizeY;
}

bool checkBounds(const SimulationStructures simStruct, int x, int y, int z) {
    return x >= 0 && x < simStruct.sizeX && y >= 0 && y < simStruct.sizeY && z >= 0 && z < simStruct.sizeZ;
}

uint getOffset(const SimulationStructures simStruct, uint x, uint y, uint z) {
    return simStruct.grid[calculatePosition(simStruct, x, y, z)].particleOffset;
}

bool isEmpty(const SimulationStructures simStruct, uint x, uint y, uint z) {
    return getOffset(simStruct, x, y, z) == 0;
}

Particle getParticle(const SimulationStructures simStruct, uint x, uint y, uint z) {
    return simStruct.particles[getOffset(simStruct, x, y, z)-1];
}

ParticleInfo getParticleInfo(const SimulationStructures simStruct, Particle particle) {
    return simStruct.particleInfo[(particle.type)-1];
}
