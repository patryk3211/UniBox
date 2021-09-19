#include <util/font.hpp>

#include <spdlog/spdlog.h>
#include <stb/stb_image.h>
#include <ostream>
#include <fstream>
#include <chrono>

using namespace unibox;
using namespace unibox::util;

FT_Library Font::library = 0;

Font::Font(const std::string& filename, unsigned int characterHeight) : atlas(0, 0, false) {
    if(library == 0) {
        FT_Error error = FT_Init_FreeType(&library);
        if(error) {
            spdlog::error("Failed to initialize the FreeType library.");
            return;
        }
        Finalizer::addCallback([]() {
            FT_Done_FreeType(library);
        });
    }

    FT_Error error = FT_New_Face(library, filename.c_str(), 0, &fontface);
    if(error) {
        spdlog::error("Failed to load font '" + filename + "'.");
        return;
    }

    error = FT_Set_Char_Size(fontface, 0, characterHeight*64, 300, 300);
    if(error) {
        spdlog::error("Failed to set character size.");
        return;
    }

    error = FT_Set_Pixel_Sizes(fontface, 0, characterHeight);
    if(error) {
        spdlog::error("Failed to set pixel size.");
        return;
    }
}

Font::~Font() {
    //FT_Done_Face(fontface);
}

const FT_Bitmap* Font::getBitmap(FT_ULong c) const {
    FT_UInt glyphIndex = FT_Get_Char_Index(fontface, c);
    FT_Error error = FT_Load_Glyph(fontface, glyphIndex, FT_LOAD_DEFAULT);
    if(error) {
        spdlog::error("Failed to load a glyph.");
        return 0;
    }
    error = FT_Render_Glyph(fontface->glyph, ft_render_mode_normal);
    if(error) {
        spdlog::error("Failed to render a glyph.");
        return 0;
    }
    
    return &fontface->glyph->bitmap;
}

void Font::bakeAtlas() {
    if(atlas.isFinished()) return;
    spdlog::info("Font texture atlas bake started.");
    auto start = std::chrono::high_resolution_clock::now();

    struct CharToResolve {
        FT_ULong character;

        VariableTextureAtlas::Coordinate coordinate;

        float left;
        float top;

        float width;
        float height;

        float advance;
    };
    std::list<CharToResolve> toResolve;

    FT_UInt idx;
    for(FT_ULong c = FT_Get_First_Char(fontface, &idx); idx != 0; c = FT_Get_Next_Char(fontface, c, &idx)) {
        const FT_Bitmap* bitmap = getBitmap(c);
        if(bitmap->buffer != 0) {
            unsigned int img[bitmap->rows*bitmap->width];
            for(int i = 0; i < bitmap->width; i++) {
                for(int j = 0; j < bitmap->rows; j++) {
                    unsigned char color = ((unsigned char*)bitmap->buffer)[i+j*bitmap->pitch];
                    img[i+j*bitmap->width] = 0xFFFFFF | (color << 24);
                }
            }
            CharToResolve res = { 
                c,
                atlas.storeTexture(bitmap->width, bitmap->rows, img),
                fontface->glyph->bitmap_left,
                fontface->glyph->bitmap_top,
                fontface->glyph->bitmap.width,
                fontface->glyph->bitmap.rows,
                fontface->glyph->advance.x/64
            };
            toResolve.push_back(res);
        } else {
            Character chara = {
                c,
                { 0, 0, 0, 0 },
                fontface->glyph->bitmap_left,
                fontface->glyph->bitmap_top,
                0,
                0,
                fontface->glyph->advance.x/64
            };
            characterMap.insert({ c, chara });
        }
    }

    spdlog::info("Finishing the texture atlas.");
    atlas.finish();
    for(auto& c : toResolve) {
        auto value = c.coordinate.resolve();
        Character chara = {
            c.character,
            value,
            c.left,
            c.top,
            c.width,
            c.height,
            c.advance
        };
        characterMap.insert({ c.character, chara });
    }
    spdlog::info("Resolved " + std::to_string(characterMap.size()) + " characters.");

    auto end = std::chrono::high_resolution_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::microseconds>(end-start);
    spdlog::info("Font texture atlas baking finished in " + std::to_string(dur.count()/1000) + "ms.");
    spdlog::info("Generated a " + std::to_string(atlas.getWidth()) + "x" + std::to_string(atlas.getHeight()) + " image.");

    spdlog::info("Writing to file...");
    std::ofstream stream = std::ofstream("output.bin", std::ios::binary);
    stream.write((char*)atlas.getAtlasData(), atlas.getWidth()*atlas.getHeight()*4);
}

const Character& Font::getCharacter(FT_ULong c) const {
    return characterMap.at(c);
}

const VariableTextureAtlas& Font::getAtlas() const {
    return atlas;
}
