void unibox_gate_update(const SimulationStructures structs, Particle* vertex) {
    if(vertex->data[0] > 0) vertex->data[0]--;
    if(vertex->data[1] > 0) vertex->data[1]--;
    else vertex->state &= ~0x01;
    bool set = false;
    // TODO: [10.09.2021] It's kinda working but I feel like this could be improved.
    for(int i = -1; i <= 1; i++) {
        for(int j = -1; j <= 1; j++) {
            if(i == 0 && j == 0) continue;
            if(!checkBounds(structs, (int)vertex->position[0]+i, (int)vertex->position[1]+j, (int)vertex->position[2])) continue;
            Particle neighbour = getParticle(structs, (uint)vertex->position[0]+i, (uint)vertex->position[1]+j, (uint)vertex->position[2]);
            if(neighbour.type == UNIBOX_SPARK && (i == 0 || j == 0)) {
                if(((neighbour.stype == UNIBOX_NSILICON && vertex->data[1] > 0) || (neighbour.stype == UNIBOX_PSILICON && vertex->data[1] == 0)) && vertex->data[0] == 0) {
                    vertex->data[0] = 2;
                    vertex->stype = vertex->type;
                    vertex->type = UNIBOX_SPARK;
                    break;
                } else if(neighbour.stype != UNIBOX_NSILICON && neighbour.stype != UNIBOX_PSILICON) vertex->data[1] = 4;
            } else if(neighbour.type == UNIBOX_GATE && neighbour.data[1] > vertex->data[1] && !(vertex->state&0x01) && !(neighbour.state&0x01)) {
                vertex->data[1] = neighbour.data[1];
                set = true;
            }
        }
    }
    if(set) vertex->state |= 0x01;
}