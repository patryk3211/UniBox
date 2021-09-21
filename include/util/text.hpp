#pragma once

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <util/font.hpp>

#include <string>
#include <vector>

namespace unibox::util {
    class Text {
        struct Vertex {
            glm::vec3 position;
            glm::vec2 uv;
        };

        struct Rect {
            Vertex vertices[6];

            Rect(glm::vec2 start, glm::vec2 end, glm::vec2 uvStart, glm::vec2 uvSize);
        };

        const Font& font;

        std::string value;

        std::vector<Rect> mesh;
        float length;
        float height;

        void generateMesh();
    public:
        Text(const Font& font, const std::string& str);
        ~Text();

        void setText(const std::string& str);
        void* getMesh();
        std::vector<unsigned char> getMeshVec();

        unsigned int getVertexCount();
        float getMeshLength();
        float getMeshHeight();
    };
}