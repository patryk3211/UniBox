#pragma once

#include <gui-engine/object.hpp>

#include <algorithm>
#include <list>

namespace unibox::gui {
    class Button : public GuiObject {
        std::list<std::function<void(double, double, int)>> clickEvents;

        gui_resource_handle normal;
        gui_resource_handle hover;
        gui_resource_handle click;

        bool enabled;
        bool visible;

        bool isClicked;
    public:
        Button(GuiEngine& engine, gui_resource_handle shader, gui_resource_handle normalTexture, gui_resource_handle hoverTexture, gui_resource_handle clickTexture, double posX, double posY, double width, double height);
        ~Button();

        void addClickEvent(std::function<void(double, double, int)> event);

        void setEnable(bool enable);
        void setVisible(bool visible);

        virtual void render(double frameTime, double x, double y);

        virtual bool mouseDown(double x, double y, int button);
        virtual bool mouseUp(double x, double y, int button);
    };
}