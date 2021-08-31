void update(inout ArrayVertex vertex) {
    const float xStart = vertex.position[0];
    const float yStart = vertex.position[1];
    const float zStart = vertex.position[2];

    const uint stepCountX = uint(abs(vertex.position[0]+vertex.velocity[0]-uint(vertex.position[0])));
    const uint stepCountY = uint(abs(vertex.position[1]+vertex.velocity[1]-uint(vertex.position[1])));
    const uint stepCountZ = uint(abs(vertex.position[2]+vertex.velocity[2]-uint(vertex.position[2])));
    uint stepCount = max(stepCountX, max(stepCountY, stepCountZ));
    if(stepCount <= 0) stepCount = 1;

    float stepX = vertex.velocity[0]/stepCount;
    float stepY = vertex.velocity[1]/stepCount;
    float stepZ = vertex.velocity[2]/stepCount;

    for(int i = 0; i < stepCount; i++) {
        const float newX = vertex.position[0]+stepX;
        const float newY = vertex.position[1]+stepY;
        const float newZ = vertex.position[2]+stepZ;
        if(uint(newX) == uint(vertex.position[0]) && uint(newY) == uint(vertex.position[1]) && uint(newZ) == uint(vertex.position[2])) {
            vertex.position[0] = newX;
            vertex.position[1] = newY;
            vertex.position[2] = newZ;
            continue;
        }

        if(checkBounds(uint(newX), uint(newY), uint(newZ))) {
            // TODO: [30.07.2021] Handle reflection correctly
            ArrayVertex neighbour = getParticle(uint(newX), uint(newY), uint(newZ));
            if((neighbour.id&0xFFFF) == UNIBOX_OPTICAL_FIBER) {
                // TODO: The photon will stop on the filter particle and the filter particle will absorb it from it's side, after that the photon will disappear
                vertex.velocity[0] = 0;
                vertex.velocity[1] = 0;
                vertex.velocity[2] = 0;
                vertex.state |= 1;
                return;
            }
            vertex.position[0] = newX;
            vertex.position[1] = newY;
            vertex.position[2] = newZ;
        } else {
            vertex.velocity[0] = -vertex.velocity[0];
            vertex.velocity[1] = -vertex.velocity[1];
            vertex.velocity[2] = -vertex.velocity[2];
        }
    }

    if((vertex.state&0x01)!=0) {
        vertex.id = 0;
        vertex.state = 0;
    }
}