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
    if((vertex->state&0x01)!=0) {
        vertex->type = 0;
        vertex->state = 0;
        return;
    }

    const float xStart = vertex->position[0];
    const float yStart = vertex->position[1];
    const float zStart = vertex->position[2];

    const uint stepCountX = (uint)(abs((uint)(vertex->position[0]+vertex->velocity[0]-(uint)vertex->position[0])));
    const uint stepCountY = (uint)(abs((uint)(vertex->position[1]+vertex->velocity[1]-(uint)vertex->position[1])));
    const uint stepCountZ = (uint)(abs((uint)(vertex->position[2]+vertex->velocity[2]-(uint)vertex->position[2])));
    uint stepCount = max(stepCountX, max(stepCountY, stepCountZ));
    if(stepCount <= 0) stepCount = 1;

    float stepX = vertex->velocity[0]/stepCount;
    float stepY = vertex->velocity[1]/stepCount;
    float stepZ = vertex->velocity[2]/stepCount;

    for(int i = 0; i < stepCount; i++) {
        const float newX = vertex->position[0]+stepX;
        const float newY = vertex->position[1]+stepY;
        const float newZ = vertex->position[2]+stepZ;
        if((uint)newX == (uint)vertex->position[0] && (uint)newY == (uint)vertex->position[1] && (uint)newZ == (uint)vertex->position[2]) {
            vertex->position[0] = newX;
            vertex->position[1] = newY;
            vertex->position[2] = newZ;
            continue;
        }

        if(checkBounds(structs, (uint)newX, (uint)newY, (uint)newZ)) {
            // TODO: [30.07.2021] Handle reflection correctly
            Particle neighbour = getParticle(structs, (uint)newX, (uint)newY, (uint)newZ);
            switch(neighbour.type) {
                case UNIBOX_OPTICAL_FIBER:
                    vertex->velocity[0] = 0;
                    vertex->velocity[1] = 0;
                    vertex->velocity[2] = 0;
                    vertex->state |= 1;
                    break;
                case 0:
                    vertex->position[0] = newX;
                    vertex->position[1] = newY;
                    vertex->position[2] = newZ;
                    break;
                default:
                    // Reflect
                    break;
            }
        } else {
            vertex->velocity[0] = -vertex->velocity[0];
            vertex->velocity[1] = -vertex->velocity[1];
            vertex->velocity[2] = -vertex->velocity[2];
        }
    }
}