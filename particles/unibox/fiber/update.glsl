bool checkNeighbour(inout ArrayVertex vertex, ArrayVertex neighbour) {
    switch((neighbour.id&0xFFFF)) {
        case UNIBOX_OPTICAL_FIBER:
            if(neighbour.data[0] == 2) {
                // TODO: [31.07.2021] Blend the color into the current color.
                vertex.data[0] = 2;
                vertex.data[1] = neighbour.data[1];
                return true;
            }
            break;
        case UNIBOX_PHOTON:
            const uint divider = uint(1/(neighbour.data[0]/4));
            const uint divider2 = uint(1/(1.0-(neighbour.data[0]/4)));
            vertex.data[1] = uint(vertex.data[1]/divider)+uint(neighbour.data[0]/divider2);
            if(vertex.data[0] == 0) vertex.data[0] = 2;
            return true;
    }
    return false;
}

void update(inout ArrayVertex vertex) {
    // Since it's a compute shader, it's better for the fiber to "pull" the light in rather than pushing it out.
    if(vertex.data[0] == 0) {
        // TODO: [31.07.2021] Add light velocity so that in case of an intersection it prefers to go a certain way.
        ArrayVertex neighbour = getParticle(uint(vertex.position[0])-1, uint(vertex.position[1]), uint(vertex.position[2]));
        if(checkNeighbour(vertex, neighbour)) return;

        neighbour = getParticle(uint(vertex.position[0])+1, uint(vertex.position[1]), uint(vertex.position[2]));
        if(checkNeighbour(vertex, neighbour)) return;

        neighbour = getParticle(uint(vertex.position[0]), uint(vertex.position[1])-1, uint(vertex.position[2]));
        if(checkNeighbour(vertex, neighbour)) return;

        neighbour = getParticle(uint(vertex.position[0]), uint(vertex.position[1])+1, uint(vertex.position[2]));
        if(checkNeighbour(vertex, neighbour)) return;
    } else {
        vertex.data[0]--;
    }
}