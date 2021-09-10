void unibox_mixLight(Particle* vertex, const Particle neighbour, uint* mixedValue, bool* received) {
    switch(neighbour.type) {
        case UNIBOX_OPTICAL_FIBER: {
            if(neighbour.data[0] == 2) {
                *mixedValue |= neighbour.data[1];
                *received = true;
            }
            break;
        } case UNIBOX_PHOTON: {
            *mixedValue |= neighbour.data[0];
            *received = true;
            vertex->data[2] = 1;
            break;
        } case UNIBOX_FILTER: {
            if(neighbour.data[0] == 2) {
                *mixedValue |= neighbour.data[1];
                *received = true;
            }
            break;
        }
    }
}