#pragma once

namespace unibox {
    struct Voxel {
        unsigned short type;
        unsigned short stype;
        float paintColor[4];
        unsigned int data[4];
        float temperature;
        float velocity[3];
        float position[3];
    };

    struct GridPoint {
        uint particleOffset;
    };
}