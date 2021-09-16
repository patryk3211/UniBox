#pragma once

namespace unibox {
    class TextureAtlas {
        unsigned int* usageMap;
        unsigned int* data;

        unsigned int width;
        unsigned int height;

        bool isFree(unsigned int x, unsigned int y);
        bool isFree(unsigned int x, unsigned int y, unsigned int width, unsigned int height);
        void setState(unsigned int x, unsigned int y, bool state);
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
    };
}