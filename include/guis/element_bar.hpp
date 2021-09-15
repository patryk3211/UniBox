#pragma once

#include <gui-engine/object.hpp>

namespace unibox {
    class ElementBar : public gui::GuiObject {
        
    public:
        ElementBar(gui::GuiEngine& engine);
        ~ElementBar();

        virtual void render(double frameTime, double x, double y);
    };
}