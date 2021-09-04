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
    const float xStart = vertex->position[0];
    const float yStart = vertex->position[1];
    const float zStart = vertex->position[2];

    const uint stepCountX = abs((uint)(vertex->position[0]+vertex->velocity[0]-(uint)vertex->position[0]));
    const uint stepCountY = abs((uint)(vertex->position[1]+vertex->velocity[1]-(uint)vertex->position[1]));
    const uint stepCountZ = abs((uint)(vertex->position[2]+vertex->velocity[2]-(uint)vertex->position[2]));
    uint stepCount = max(stepCountX, max(stepCountY, stepCountZ));
    if(stepCount <= 0) stepCount = 1;

    const float stepX = vertex->velocity[0]/stepCount;
    const float stepY = vertex->velocity[1]/stepCount;
    const float stepZ = vertex->velocity[2]/stepCount;

    for(int i = 0; i < stepCount; i++) {
        const float newX = vertex->position[0]+stepX;
        const float newY = vertex->position[1]+stepY;
        const float newZ = vertex->position[2]+stepZ;
        if((int)newX == (int)vertex->position[0] && (int)newY == (int)vertex->position[1] && (int)newZ == (int)vertex->position[2]) {
            vertex->position[0] = newX;
            vertex->position[1] = newY;
            vertex->position[2] = newZ;
            continue;
        }

        if(checkBounds(structs, (int)newX, (int)newY, (int)newZ)) {
            if(isEmpty(structs, (uint)newX, (uint)newY, (uint)newZ)) {
                vertex->position[0] = newX;
                vertex->position[1] = newY;
                vertex->position[2] = newZ;
                continue;
            }
            const float newNewXp = newX+stepY;
            const float newNewYp = newY-stepX;
            if((int)newNewXp != (int)newX || (int)newNewYp != (int)newY) {
                if(checkBounds(structs, (int)newNewXp, (int)newNewYp, (int)newZ) && isEmpty(structs, (uint)newNewXp, (uint)newNewYp, (uint)newZ)) {
                    vertex->position[0] = newNewXp;
                    vertex->position[1] = newNewYp;
                    vertex->position[2] = newZ;
                    continue;
                }
            }
            const float newNewXn = newX-stepY;
            const float newNewYn = newY+stepX;
            if((int)newNewXn != (int)newX || (int)newNewYn != (int)newY) {
                if(checkBounds(structs, (int)newNewXn, (int)newNewYn, (int)newZ) && isEmpty(structs, (uint)newNewXn, (uint)newNewYn, (uint)newZ)) {
                    vertex->position[0] = newNewXn;
                    vertex->position[1] = newNewYn;
                    vertex->position[2] = newZ;
                    continue;
                }
            }
        }
    }

    if(vertex->position[0] == xStart && vertex->position[1] == yStart && vertex->position[2] == zStart &&
      (vertex->velocity[0] != 0 || vertex->velocity[1] != 0 || vertex->velocity[2] != 0))
        vertex->state |= 0x01;
    else vertex->state &= 0xFFFFFFFE;

    if((vertex->state & 0x01) != 0) {
        vertex->state &= 0xFFFFFFFE;
        vertex->velocity[0] *= 0.9;
        vertex->velocity[1] *= 0.9;
        vertex->velocity[2] *= 0.9;
    }
}