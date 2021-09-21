#include <util/text.hpp>

using namespace unibox;
using namespace unibox::util;

Text::Rect::Rect(glm::vec2 start, glm::vec2 end, glm::vec2 uvStart, glm::vec2 uvSize) {
    vertices[0].position = { start.x, start.y, 0 };
    vertices[1].position = { start.x, end.y,   0 };
    vertices[2].position = { end.x,   start.y, 0 };
    vertices[3].position = { end.x,   start.y, 0 };
    vertices[4].position = { start.x, end.y,   0 };
    vertices[5].position = { end.x,   end.y,   0 };

    vertices[0].uv = { uvStart.x,          uvStart.y          };
    vertices[1].uv = { uvStart.x,          uvStart.y+uvSize.y };
    vertices[2].uv = { uvStart.x+uvSize.x, uvStart.y          };
    vertices[3].uv = { uvStart.x+uvSize.x, uvStart.y          };
    vertices[4].uv = { uvStart.x,          uvStart.y+uvSize.y };
    vertices[5].uv = { uvStart.x+uvSize.x, uvStart.y+uvSize.y };
}

Text::Text(const Font& font, const std::string& str) : font(font), value(str) { generateMesh(); }
Text::~Text() { }

void Text::generateMesh() {
    mesh.clear();
    glm::vec2 offset = glm::vec2(0, 0);
    height = 0;
    for(int i = 0; i < value.size(); i++) {
        auto& c = font.getCharacter(value[i]);
        glm::vec2 startPos = offset + glm::vec2(c.left, -c.top);
        glm::vec2 endPos = startPos + glm::vec2(c.width, c.height);
        Rect rect = Rect(startPos, endPos, glm::vec2(c.coords.x, c.coords.y), glm::vec2(c.coords.width, c.coords.height));
        mesh.push_back(rect);
        offset.x += c.advance;
    }
    length = offset.x;
}

void Text::setText(const std::string& str) {
    this->value = str;
    generateMesh();
}

void* Text::getMesh() {
    return mesh.data();
}

std::vector<unsigned char> Text::getMeshVec() {
    std::vector<unsigned char> data;
    for(auto& rect : mesh) {
        unsigned char* byteData = (unsigned char*)&rect;
        for(int i = 0; i < sizeof(rect); i++) data.push_back(byteData[i]);
    }
    return data;
}

unsigned int Text::getVertexCount() {
    return mesh.size()*6;
}

float Text::getMeshLength() {
    return length;
}

float Text::getMeshHeight() {
    return height;
}
