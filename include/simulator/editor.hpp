#pragma once

#include <simulator/particle_grid.hpp>
#include <simulator/particle.hpp>

namespace unibox {
    class GridEditor {
        ParticleGrid* ref;

        Particle** toolbar;
    
    public:
        GridEditor(ParticleGrid* grid);
        ~GridEditor();
    };
}