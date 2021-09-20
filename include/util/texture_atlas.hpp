#pragma once

#include <spdlog/spdlog.h>

#include <vector>
#include <list>

namespace unibox {
    class TextureAtlas {
        unsigned int* usageMap;
        unsigned int* data;

        unsigned int width;
        unsigned int height;

        bool isFree(unsigned int x, unsigned int y);
        bool isFree(unsigned int x, unsigned int y, unsigned int width, unsigned int height);
        void setState(unsigned int x, unsigned int y, bool state);

        bool isModifiable;
    public:
        struct Coordinate {
            float x;
            float y;
            float width;
            float height;

            Coordinate();
            Coordinate(float x, float y, float width, float height);
        };

        TextureAtlas(unsigned int width, unsigned int height);
        ~TextureAtlas();

        Coordinate storeTexture(unsigned int width, unsigned int height, void* data);
        void* getAtlasData();

        void finish();
    };

    template<typename T> class VariableTextureAtlas {
        struct Point {
            unsigned int x;
            unsigned int y;

            bool operator==(const Point& other) const {
                return other.x == x && other.y == y;
            }
        };

        struct UsageLine {
            unsigned int firstFreeIdx;
            std::vector<unsigned char> freeBitmap;

            UsageLine() :
            firstFreeIdx(0) { }

            UsageLine(unsigned int size) :
            freeBitmap(size),
            firstFreeIdx(0) { }

            void resize(unsigned int size) { freeBitmap.resize(size); }
        };
        
        std::vector<std::vector<T>> data;
        std::vector<UsageLine> freeMap;
        T* finishedData;

        unsigned int width;
        unsigned int height;

        bool isFree(unsigned int x, unsigned int y) {
            if(!modifiable) return false;
            if(y >= height || x >= width) return false;
            return freeMap[y].freeBitmap[x] == 0;
        }

        bool isFree(unsigned int x, unsigned int y, unsigned int width, unsigned int height) {
            if(!modifiable) return false;
            for(unsigned int i = 0; i < width; i++) {
                for(unsigned int j = 0; j < height; j++) {
                    if(!isFree(x+i, y+j)) return false;
                }
            }
            return true;
        }

        void enlarge(unsigned int deltaX, unsigned int deltaY) {
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

        bool modifiable;
        bool precisePacking;
    public:
        struct Coordinate {
            const VariableTextureAtlas* atlas;

            unsigned int x;
            unsigned int y;
            unsigned int width;
            unsigned int height;

            TextureAtlas::Coordinate resolve() const {
                if(atlas->modifiable) spdlog::warn("Resolving texture coordinates for an unfinished atlas, the coordinates may not be valid after a texture insertion.");
                // Add half a pixel to the coordinates so that bad stuff doesn't happen.
                TextureAtlas::Coordinate coord = { (x+.5f)/(float)atlas->width, (y+.5f)/(float)atlas->height, (width-1)/(float)atlas->width, (height-1)/(float)atlas->height };
                return coord;
            }
        };

        VariableTextureAtlas(unsigned int initialWidth, unsigned int initialHeight, bool precisePacking) {
            this->width = 0;
            this->height = 0;

            modifiable = true;
            enlarge(initialWidth, initialHeight);

            finishedData = 0;
            this->precisePacking = precisePacking;
        }
        ~VariableTextureAtlas() {
            if(finishedData != 0) delete[] finishedData;
        }

        Coordinate storeTexture(unsigned int width, unsigned int height, const void* data) {
            if(!modifiable) return { 0, 0, 0, 0 };
            const T* data_t = (const T*)data;
            for(unsigned int y = 0; y < this->height; y++) {
                for(unsigned int x = freeMap[y].firstFreeIdx; x < this->width; x++) {
                    if(!isFree(x, y, width, height)) continue;
                    // Found a free pos
                    for(int j = 0; j < height; j++) {
                        for(int i = 0; i < width; i++) {
                            this->data[y+j][x+i] = data_t[i+j*width];
                            Point p = { x+i, y+j };
                            freeMap[y+j].freeBitmap[x+i] = 1;
                            if(!precisePacking) freeMap[y+j].firstFreeIdx = x+i;
                        }
                        // Recalculate the firstFreeIdx for the line.
                        if(precisePacking) {
                            for(int i = 0; i < this->width; i++) {
                                if(freeMap[y+j].freeBitmap[i] == 0) {
                                    freeMap[y+j].firstFreeIdx = i;
                                    break;
                                }
                            }
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

        void* getAtlasData() const {
            if(modifiable) {
                spdlog::error("Cannot take the data of an unfinished variable texture atlas.");
                return 0;
            }
            return finishedData;
        }

        unsigned int getWidth() const { return width; }
        unsigned int getHeight() const { return height; }

        void finish() {
            if(!modifiable) return;
            modifiable = false;
            freeMap.clear();
            finishedData = new T[width*height];
            for(unsigned int j = 0; j < height; j++) {
                for(unsigned int i = 0; i < width; i++) {
                    finishedData[i+j*width] = data[j][i];
                }
            }
        }
        bool isFinished() const { return !modifiable; }
    };
}