#include <gui-engine/button.hpp>

#include <gui-engine/engine.hpp>

#include <spdlog/spdlog.h>

using namespace unibox::gui;

Button::Button(GuiEngine& engine, gui_resource_handle shader, gui_resource_handle normalTexture, gui_resource_handle hoverTexture, gui_resource_handle clickTexture, double posX, double posY, double width, double height) : 
    GuiObject(engine, shader, 0, posX, posY, width, height) {
    this->normal = normalTexture;
    this->hover = hoverTexture;
    this->click = clickTexture;

    this->enabled = true;
    this->visible = true;
}

Button::~Button() {

}

void Button::addClickEvent(std::function<void(double, double, int)> event) {
    clickEvents.push_back(event);
}

void Button::setEnable(bool enable) {
    this->enabled = enable;
}

void Button::setVisible(bool visible) {
    this->visible = visible;
}

void Button::render(double frameTime, double x, double y) {
    if(visible) {
        if(isClicked) {

        } else {
            if(x >= getX()-getWidth()/2 && y >= getY()-getHeight()/2 && x <= getX()+getWidth()/2 && y <= getY()+getHeight()/2) renderEngine.bind_texture_to_descriptor(getRenderObject(), "texture0", hover);
            else renderEngine.bind_texture_to_descriptor(getRenderObject(), "texture0", normal);
        }
        GuiObject::render(frameTime, x, y);
    }
}

void Button::mouseClick(double x, double y, int button) {

}
