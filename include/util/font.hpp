#pragma once

#include <glm/vec2.hpp>
#include <util/finalizer.hpp>
#include <vk-engine/image.hpp>
#include <util/texture_atlas.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <string>
#include <unordered_map>

namespace unibox::util {
    struct Character {
        FT_ULong character;
        TextureAtlas::Coordinate coords;

        float left;
        float top;

        float width;
        float height;

        float advance;
        float advanceY;
    };

    class Font {
        static FT_Library library;

        FT_Face fontface;
        std::unordered_map<FT_ULong, Character> characterMap;

        VariableTextureAtlas<unsigned char> atlas;

        float fontSize;
    public:
        Font(const std::string& filename, unsigned int characterHeight);
        ~Font();

        const FT_Bitmap* getBitmap(FT_ULong c) const;
        const Character& getCharacter(FT_ULong c) const;

        const VariableTextureAtlas<unsigned char>& getAtlas() const;

        void bakeAtlas();

        float getFontSize() const;
    };
}