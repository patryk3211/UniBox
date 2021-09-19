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
                return { i/(float)this->width, j/(float)this->height, width/(float)this->width, height/(float)this->height };
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



bool VariableTextureAtlas::isFree(unsigned int x, unsigned int y) {
    if(!modifiable) return false;
    if(y >= height || x >= width) return false;
    return freeMap[y].freeBitmap[x] == 0;
}

bool VariableTextureAtlas::isFree(unsigned int x, unsigned int y, unsigned int width, unsigned int height) {
    if(!modifiable) return false;
    for(unsigned int i = 0; i < width; i++) {
        for(unsigned int j = 0; j < height; j++) {
            if(!isFree(x+i, y+j)) return false;
        }
    }
    return true;
}

TextureAtlas::Coordinate VariableTextureAtlas::Coordinate::resolve() {
    if(atlas->modifiable) spdlog::warn("Resolving texture coordinates for an unfinished atlas, the coordinates may not be valid after a texture insertion.");
    TextureAtlas::Coordinate coord = { x/(float)atlas->width, y/(float)atlas->height, width/(float)atlas->width, height/(float)atlas->height };
    return coord;
}

void VariableTextureAtlas::enlarge(unsigned int deltaX, unsigned int deltaY) {
    if(!modifiable) return;
    freeMap.resize(height+deltaY);
    for(int j = 0; j < height+deltaY; j++) {
        freeMap[j].freeBitmap.resize(width+deltaX);
    }
    this->width += deltaX;
    this->height += deltaY;
    data.resize(height);
    for(int y = 0; y < height; y++) data[y].resize(width);
}

VariableTextureAtlas::VariableTextureAtlas(unsigned int initialWidth, unsigned int initialHeight) {
    this->width = 0;
    this->height = 0;

    modifiable = true;
    enlarge(initialWidth, initialHeight);

    finishedData = 0;
}

VariableTextureAtlas::~VariableTextureAtlas() {
    if(finishedData != 0) delete[] finishedData;
}

VariableTextureAtlas::Coordinate VariableTextureAtlas::storeTexture(unsigned int width, unsigned int height, const void* data) {
    if(!modifiable) return { 0, 0, 0, 0 };
    const unsigned int* data_i = (const unsigned int*)data;
    for(unsigned int y = 0; y < this->height; y++) {
        for(unsigned int x = freeMap[y].firstFreeIdx; x < this->width; x++) {
            if(!isFree(x, y, width, height)) continue;
            // Found a free pos
            for(int j = 0; j < height; j++) {
                for(int i = 0; i < width; i++) {
                    this->data[y+j][x+i] = data_i[i+j*width];
                    Point p = { x+i, y+j };
                    freeMap[y+j].freeBitmap[x+i] = 1;
                    freeMap[y+j].firstFreeIdx = x+i;
                }
            }
            return { this, x, y, width, height };
        }
    }

    // Did not find a viable position, we have to enlarge the atlas.
    unsigned int newWidth = this->width+width;
    unsigned int newHeight = this->height+height;
    
    // Find the powers of 2 of the new width and height.
    unsigned int powWidth = 1;
    while(powWidth < newWidth) powWidth <<= 1;
    unsigned int powHeight = 1;
    while(powHeight < newHeight) powHeight <<= 1;

    if(powWidth != powHeight) {
        if(powWidth > powHeight) powHeight = powWidth;
        else powWidth = powHeight;
    }

    enlarge(powWidth-this->width, powHeight-this->height); // Enlarge the atlas.
    return storeTexture(width, height, data); // Try to store again.
}

void* VariableTextureAtlas::getAtlasData() {
    if(modifiable) {
        spdlog::error("Cannot take the data of an unfinished variable texture atlas.");
        return 0;
    }
    return finishedData;
}

void VariableTextureAtlas::finish() {
    if(!modifiable) return;
    modifiable = false;
    freeMap.clear();
    finishedData = new unsigned int[width*height];
    for(unsigned int j = 0; j < height; j++) {
        for(unsigned int i = 0; i < width; i++) {
            finishedData[i+j*width] = data[j][i];
        }
    }
}

unsigned int VariableTextureAtlas::getWidth() {
    return width;
}

unsigned int VariableTextureAtlas::getHeight() {
    return height;
}