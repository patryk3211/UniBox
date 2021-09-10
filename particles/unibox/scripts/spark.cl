void conductorUpdate(const SimulationStructures structs, Particle* vertex) {
    if(vertex->data[0] == 0) {
        for(int i = -1; i <= 1; i++) {
            for(int j = -1; j <= 1; j++) {
                if(i == 0 && j == 0) continue;
                if(!checkBounds(structs, (int)vertex->position[0]+i, (int)vertex->position[1]+j, (int)vertex->position[2])) continue;
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
