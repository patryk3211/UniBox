#pragma once

namespace unibox {
    struct Voxel {
        unsigned short type;
        unsigned short stype;
        unsigned int state;
        float paintColor[4];
        unsigned int data[4];
    };
}