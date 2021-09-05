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

void update(const SimulationStructures structs, Particle* vertex) {
    if(vertex->data[0] == 0) {
        for(int i = -1; i <= 1; i++) {
            for(int j = -1; j <= 1; j++) {
                if(i == 0 && j == 0) continue;
                Particle neighbour = getParticle(structs, (uint)vertex->position[0]+i, (uint)vertex->position[1]+j, (uint)vertex->position[2]);
                if(neighbour.type == UNIBOX_SPARK) {
                    vertex->data[0] = 2;
                    vertex->stype = vertex->type;
                    vertex->type = UNIBOX_SPARK;
                    break;
                }
            }
        }
    } else {
        vertex->data[0]--;
    }
}