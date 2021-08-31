#include <util/savefile.hpp>

#include <spdlog/spdlog.h>

using namespace unibox;

SaveFile::SaveFile(const std::string& file) {
    valid = false;
    fileStream = std::ifstream(file, std::ios::binary);
    char magic[4];
    fileStream.read(magic, 4);
    if(magic[0] == 'U' &&
       magic[1] == 'N' &&
       magic[2] == 'B' &&
       magic[3] == 'X') {
        fileStream.read(reinterpret_cast<char*>(&version), 1);
        {
            uint8_t strLen;
            fileStream.read((char*)&strLen, 1);
            char name[strLen];
            fileStream.read(name, strLen);
            this->name = name;
        } {
            uint16_t descLen;
            fileStream.read((char*)&descLen, 2);
            char description[descLen];
            fileStream.read(description, descLen);
            this->description = description;
        } {
            uint32_t data[3];
            fileStream.read((char*)data, sizeof(uint32_t)*3);
            sizeX = data[0];
            sizeY = data[1];
            sizeZ = data[2];
        }
        valid = true;
    }
}

SaveFile::~SaveFile() {

}

void SaveFile::particleReadVersion1() {
    {
        uint16_t mappingsLen;
        fileStream.read((char*)&mappingsLen, 2);
        for(int i = 0; i < mappingsLen; i++) {
            uint8_t nameLen;
            fileStream.read((char*)&nameLen, 1);
            char name[nameLen+1];
            fileStream.read(name, nameLen);
            name[nameLen] = 0;

            uint16_t id;
            fileStream.read((char*)&id, 2);
            mappings.insert({ id, Particle::getParticleId(name) });
        }
    } {
        uint32_t particleCount;
        fileStream.read((char*)&particleCount, 4);
        for(uint32_t i = 0; i < particleCount; i++) {
            uint8_t flags;
            fileStream.read((char*)&flags, 1);

            Voxel voxel = {};
            uint16_t type;
            fileStream.read((char*)&type, 2);
            voxel.type = mappings[type];
            fileStream.read((char*)&voxel.temperature, sizeof(float));
            uint32_t position[3];
            fileStream.read((char*)&position, sizeof(position));
            voxel.position[0] = position[0];
            voxel.position[1] = position[1];
            voxel.position[2] = position[2];

            if(flags & 0x01) {
                // Include velocity
                fileStream.read((char*)&voxel.velocity, sizeof(voxel.velocity));
            }
            if(flags & 0x02) {
                // Include data
                uint16_t stype;
                fileStream.read((char*)&stype, 2);
                voxel.stype = mappings[stype];
                fileStream.read((char*)&voxel.data, sizeof(voxel.data));
            }
            if(flags & 0x04) {
                // Include paint
                uint8_t colors[4];
                fileStream.read((char*)&colors, sizeof(colors));
                voxel.paintColor[0] = colors[0]/255.0;
                voxel.paintColor[1] = colors[1]/255.0;
                voxel.paintColor[2] = colors[2]/255.0;
                voxel.paintColor[3] = colors[3]/255.0;
            }

            particles.push_back(voxel);
        }
    }
    fileStream.close();
}

void SaveFile::readParticles() {
    switch(version) {
        case 1: particleReadVersion1(); break;
        default: spdlog::error("Unknown particle file version."); break;
    }
}

uint32_t SaveFile::getSizeX() {
    return sizeX;
}

uint32_t SaveFile::getSizeY() {
    return sizeY;
}

uint32_t SaveFile::getSizeZ() {
    return sizeZ;
}