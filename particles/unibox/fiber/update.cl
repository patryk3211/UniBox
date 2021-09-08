#pragma COMPILATION_REMOVE
// This is here to stop the plugin from screaming that Particle is not a type.
typedef struct {
    ushort type;
    ushort stype;
    float paintColor[4];
    int data[4];
    float temperature;
    float velocity[3];
    float position[3];
    uint state;
} Particle;

typedef struct {
    uint particleOffset;
} GridPoint;

typedef struct {
    float density;
    uint state;
} ParticleInfo;

typedef struct {
    global Particle* particles;
    global GridPoint* grid;
    global ParticleInfo* particleInfo;

    uint sizeX;
    uint sizeY;
    uint sizeZ;
    uint particleCount;
} SimulationStructures;

#pragma END_COMPILATION_REMOVE

void checkNeighbour(Particle* vertex, const Particle neighbour, uint* mixedValue, bool* received) {
    switch(neighbour.type) {
        case UNIBOX_OPTICAL_FIBER: {
            if(neighbour.data[0] == 2) {
                *mixedValue |= neighbour.data[1];
                *received = true;
            }
            break;
        } case UNIBOX_PHOTON: {
            *mixedValue |= neighbour.data[0];
            *received = true;
            vertex->data[2] = 1;
            break;
        } case UNIBOX_FILTER: {
            if(neighbour.data[0] == 2) {
                *mixedValue |= neighbour.data[1];
                *received = true;
            }
            break;
        }
    }
}

void update(const SimulationStructures structs, Particle* vertex) {
    // Since it's a compute shader, it's better for the fiber to "pull" the light in rather than pushing it out.
    if(vertex->data[0] == 0) {
        // TODO: [31.07.2021] Add light velocity so that in case of an intersection it prefers to go a certain way.
        uint mixedValue = 0;
        bool received = false;
        checkNeighbour(vertex, getParticle(structs, (uint)vertex->position[0]-1, (uint)vertex->position[1], (uint)vertex->position[2]), &mixedValue, &received);
        checkNeighbour(vertex, getParticle(structs, (uint)vertex->position[0]+1, (uint)vertex->position[1], (uint)vertex->position[2]), &mixedValue, &received);
        checkNeighbour(vertex, getParticle(structs, (uint)vertex->position[0], (uint)vertex->position[1]-1, (uint)vertex->position[2]), &mixedValue, &received);
        checkNeighbour(vertex, getParticle(structs, (uint)vertex->position[0], (uint)vertex->position[1]+1, (uint)vertex->position[2]), &mixedValue, &received);

        if(received) {
            vertex->data[0] = 2;
            vertex->data[1] = mixedValue;
        }
    } else {
        vertex->data[0]--;
    }
}