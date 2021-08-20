void update(inout ArrayVertex vertex) {
    const float xStart = vertex.position[0];
    const float yStart = vertex.position[1];
    const float zStart = vertex.position[2];

    const uint stepCountX = uint(abs(vertex.position[0]+vertex.velocity[0]-uint(vertex.position[0])));
    const uint stepCountY = uint(abs(vertex.position[1]+vertex.velocity[1]-uint(vertex.position[1])));
    const uint stepCountZ = uint(abs(vertex.position[2]+vertex.velocity[2]-uint(vertex.position[2])));
    uint stepCount = max(stepCountX, max(stepCountY, stepCountZ));
    if(stepCount <= 0) stepCount = 1;

    const float stepX = vertex.velocity[0]/stepCount;
    const float stepY = vertex.velocity[1]/stepCount;
    const float stepZ = vertex.velocity[2]/stepCount;

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
            if(isEmpty(uint(newX), uint(newY), uint(newZ))) {
                vertex.position[0] = newX;
                vertex.position[1] = newY;
                vertex.position[2] = newZ;
                continue;
            }
            const float newNewXp = newX+stepY;
            const float newNewYp = newY-stepX;
            if(uint(newNewXp) != uint(newX) || uint(newNewYp) != uint(newY)) {
                if(checkBounds(uint(newNewXp), uint(newNewYp), uint(newZ)) && isEmpty(uint(newNewXp), uint(newNewYp), uint(newZ))) {
                    vertex.position[0] = newNewXp;
                    vertex.position[1] = newNewYp;
                    vertex.position[2] = newZ;
                    continue;
                }
            }
            const float newNewXn = newX-stepY;
            const float newNewYn = newY+stepX;
            if(uint(newNewXn) != uint(newX) || uint(newNewYn) != uint(newY)) {
                if(checkBounds(uint(newNewXn), uint(newNewYn), uint(newZ)) && isEmpty(uint(newNewXn), uint(newNewYn), uint(newZ))) {
                    vertex.position[0] = newNewXn;
                    vertex.position[1] = newNewYn;
                    vertex.position[2] = newZ;
                    continue;
                }
            }
        }
    }

    if(vertex.position[0] == xStart && vertex.position[1] == yStart && vertex.position[2] == zStart &&
      (vertex.velocity[0] != 0 || vertex.velocity[1] != 0 || vertex.velocity[2] != 0))
        vertex.state |= 0x01;
    else vertex.state &= 0xFFFFFFFE;

    if((vertex.state & 0x01) != 0) {
        vertex.state &= 0xFFFFFFFE;
        vertex.velocity[0] *= 0.9;
        vertex.velocity[1] *= 0.9;
        vertex.velocity[2] *= 0.9;
    }
}