#pragma once

#include <gui-engine/types.hpp>

namespace unibox {
    class GuiEngine;

    class GuiObject {
        GuiEngine* engine;

        gui_resource_handle handle;

        double posX;
        double posY;

        double scaleX;
        double scaleY;

        int layer;
    public:
        GuiObject(GuiEngine& engine, int layer, double posX, double posY, double scaleX, double scaleY);
        ~GuiObject();

        int getLayer();

        double getX();
        double getY();
        double getWidth();
        double getHeight();

        virtual void render(double frameTime);

        virtual void mouseHover(double x, double y);
        virtual void mouseClick(double x, double y, int button);
    };
}
