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

bool checkNeighbour(Particle* vertex, const Particle neighbour) {
    switch(neighbour.type) {
        case UNIBOX_OPTICAL_FIBER:
            if(neighbour.data[0] == 2) {
                if(vertex->data[0] == 0) {
                    vertex->data[1] = (uint)(neighbour.data[1]);
                    vertex->data[0] = 2;
                    return true;
                } else {
                    vertex->data[1] = (uint)(vertex->data[1]/2)+(uint)(neighbour.data[1]/2);
                    return true;
                }
            }
            break;
        case UNIBOX_PHOTON: {
            const uint divider = (uint)(1/(neighbour.data[0]/4));
            const uint divider2 = (uint)(1/(1.0-(neighbour.data[0]/4)));
            vertex->data[1] = (uint)(vertex->data[1]/divider)+(uint)(neighbour.data[0]/divider2);
            if(vertex->data[0] == 0) vertex->data[0] = 2;
            return true;
        }
    }
    return false;
}

void update(const SimulationStructures structs, Particle* vertex) {
    // Since it's a compute shader, it's better for the fiber to "pull" the light in rather than pushing it out.
    if(vertex->data[0] == 0) {
        // TODO: [31.07.2021] Add light velocity so that in case of an intersection it prefers to go a certain way.
        checkNeighbour(vertex, getParticle(structs, (uint)vertex->position[0]-1, (uint)vertex->position[1], (uint)vertex->position[2]));
        checkNeighbour(vertex, getParticle(structs, (uint)vertex->position[0]+1, (uint)vertex->position[1], (uint)vertex->position[2]));
        checkNeighbour(vertex, getParticle(structs, (uint)vertex->position[0], (uint)vertex->position[1]-1, (uint)vertex->position[2]));
        checkNeighbour(vertex, getParticle(structs, (uint)vertex->position[0], (uint)vertex->position[1]+1, (uint)vertex->position[2]));
    } else {
        vertex->data[0]--;
    }
}