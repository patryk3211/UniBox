bool fiber_checkNeighbour(inout ArrayVertex vertex, ArrayVertex neighbour) {
    if((neighbour.id&0xFFFF) == UNIBOX_OPTICAL_FIBER) {
        if(neighbour.data[0] == 2) {
            vertex.data[0] = 2;
            vertex.data[1] = neighbour.data[1];
            return true;
        }
    }
    return false;
}

void update(inout ArrayVertex vertex) {
    // Since it's a compute shader, it's better for the fiber to "pull" the light in rather than pushing it out.
    if(vertex.data[0] == 0) {
        ArrayVertex neighbour = getParticle(uint(vertex.position[0])-1, uint(vertex.position[1]), uint(vertex.position[2]));
        if(fiber_checkNeighbour(vertex, neighbour)) return;

        neighbour = getParticle(uint(vertex.position[0])+1, uint(vertex.position[1]), uint(vertex.position[2]));
        if(fiber_checkNeighbour(vertex, neighbour)) return;

        neighbour = getParticle(uint(vertex.position[0]), uint(vertex.position[1])-1, uint(vertex.position[2]));
        if(fiber_checkNeighbour(vertex, neighbour)) return;

        neighbour = getParticle(uint(vertex.position[0]), uint(vertex.position[1])+1, uint(vertex.position[2]));
        if(fiber_checkNeighbour(vertex, neighbour)) return;
    } else {
        vertex.data[0]--;
    }
}