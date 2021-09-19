#pragma once

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

    class VariableTextureAtlas { // TODO: I think this could be optimized with a quad tree, but is it really worth it?
        struct Point {
            unsigned int x;
            unsigned int y;

            bool operator==(const Point& other) const {
                return other.x == x && other.y == y;
            }
        };
        
        std::vector<std::vector<unsigned int>> data;
        std::list<Point> freeMap;
        unsigned int* finishedData;

        unsigned int width;
        unsigned int height;

        bool isFree(unsigned int x, unsigned int y);
        bool isFree(unsigned int x, unsigned int y, unsigned int width, unsigned int height);

        void enlarge(unsigned int deltaX, unsigned int deltaY);

        bool modifiable;
    public:
        struct Coordinate {
            const VariableTextureAtlas* atlas;

            unsigned int x;
            unsigned int y;
            unsigned int width;
            unsigned int height;

            TextureAtlas::Coordinate resolve();
        };

        VariableTextureAtlas(unsigned int initialWidth, unsigned int initialHeight);
        ~VariableTextureAtlas();

        Coordinate storeTexture(unsigned int width, unsigned int height, const void* data);
        void* getAtlasData();

        void finish();
    };
}