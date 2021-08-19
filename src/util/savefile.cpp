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
            char data[32];
            fileStream.read(data, 32);
            uint16_t typeId = *(uint16_t*)(data);
            uint16_t stypeId = *(uint16_t*)(data+2);
            float* dataf = (float*)(data+4);
            uint32_t* datai = (uint32_t*)(data+20);
            
            Voxel voxel = {};
            voxel.type = mappings[typeId];
            voxel.stype = mappings[stypeId];

            voxel.temperature = dataf[0];

            voxel.velocity[0] = dataf[1];
            voxel.velocity[1] = dataf[2];
            voxel.velocity[2] = dataf[3];

            voxel.position[0] = datai[0];
            voxel.position[1] = datai[1];
            voxel.position[2] = datai[2];

            particles.push_back(voxel);
        }
    }
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