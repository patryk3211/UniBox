#include <util/texture_atlas.hpp>

#include <spdlog/spdlog.h>

using namespace unibox;

TextureAtlas::Coordinate::Coordinate() {
    this->x = 0;
    this->y = 0;
    this->width = 0;
    this->height = 0;
}

TextureAtlas::Coordinate::Coordinate(float x, float y, float width, float height) {
    this->x = x;
    this->y = y;
    this->width = width;
    this->height = height;
}

TextureAtlas::TextureAtlas(unsigned int width, unsigned int height) {
    this->width = width;
    this->height = height;

    this->usageMap = new unsigned int[width*height/32];
    this->data = new unsigned int[width*height];

    for(int i = 0; i < width*height/32; i++) this->usageMap[i] = 0;

    isModifiable = true;
}

TextureAtlas::~TextureAtlas() {
    delete[] this->data;
    if(this->usageMap != 0) delete[] this->usageMap;
}

void TextureAtlas::setState(unsigned int x, unsigned int y, bool state) {
    if(x >= width || y >= height || !isModifiable) return;
    unsigned int index = x + y * width;
    if(state) this->usageMap[index/32] |= (1 << (index%32));
    else this->usageMap[index/32] &= ~(1 << (index%32));
}

bool TextureAtlas::isFree(unsigned int x, unsigned int y) {
    if(x >= width || y >= height || !isModifiable) return false;
    unsigned int index = x + y * width;
    unsigned int data = this->usageMap[index/32];
    return !((data >> (index%32)) & 1);
}

bool TextureAtlas::isFree(unsigned int x, unsigned int y, unsigned int width, unsigned int height) {
    if(!isModifiable) return false;
    for(unsigned int i = 0; i < width; i++) {
        for(unsigned int j = 0; j < height; j++) {
            if(!isFree(x+i, y+j)) return false;
        }
    }
    return true;
}

TextureAtlas::Coordinate TextureAtlas::storeTexture(unsigned int width, unsigned int height, void* data) {
    if(!isModifiable) return { 0, 0, 0, 0 };
    unsigned int* data_int = (unsigned int*)data;
    for(unsigned int i = 0; i < this->width; i++) {
        for(unsigned int j = 0; j < this->height; j++) {
            if(isFree(i, j, width, height)) {
                // Found our texture position.
                for(int x = 0; x < width; x++) {
                    for(int y = 0; y < height; y++) {
                        setState(i+x, j+y, true);
                        this->data[i+x + (j+y) * this->width] = data_int[x + y * width];
                    }
                }
                // Add half a pixel so that bad stuff doesn't happen.
                return { (i+.5f)/(float)this->width, (j+.5f)/(float)this->height, (width-1)/(float)this->width, (height-1)/(float)this->height };
            }
        }
    }
    return { 0, 0, 0, 0 };
}

void* TextureAtlas::getAtlasData() {
    return data;
}

void TextureAtlas::finish() {
    isModifiable = false;
    delete usageMap;
    usageMap = 0;
}
