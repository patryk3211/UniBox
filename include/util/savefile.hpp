#pragma once

#include <simulator/particle.hpp>

#include <string>
#include <unordered_map>
#include <istream>
#include <fstream>

namespace unibox {
    class SaveFile {
        bool valid;

        std::string name;
        std::string description;
        std::unordered_map<uint16_t, uint16_t> mappings;
        std::vector<Voxel> particles;
        uint8_t version;

        uint32_t sizeX;
        uint32_t sizeY;
        uint32_t sizeZ;
    
        std::ifstream fileStream;

        void particleReadVersion1();
    public:
        SaveFile(const std::string& file);
        ~SaveFile();

        void readParticles();
        std::vector<Voxel>& getParticles() { return particles; }

        uint32_t getSizeX();
        uint32_t getSizeY();
        uint32_t getSizeZ();
    };
}