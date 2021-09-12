#include <gui-engine/object.hpp>

#include <gui-engine/engine.hpp>

using namespace unibox;

GuiObject::GuiObject(GuiEngine& engine, int layer, double posX, double posY, double scaleX, double scaleY) {
    this->engine = &engine;

    this->layer = layer;

    this->posX = posX;
    this->posY = posY;

    this->scaleX = scaleX;
    this->scaleY = scaleY;

    handle = engine.createRenderObject();
}

GuiObject::~GuiObject() {
    engine->destroyRenderObject(handle);
}

int GuiObject::getLayer() {
    return layer;
}

void GuiObject::render(double frameTime) {

}

void GuiObject::mouseHover(double x, double y) {

}

void GuiObject::mouseClick(double x, double y, int button) {

}

double GuiObject::getX() {
    return posX;
}

double GuiObject::getY() {
    return posY;
}

double GuiObject::getWidth() {
    return scaleX;
}

double GuiObject::getHeight() {
    return scaleY;
}
