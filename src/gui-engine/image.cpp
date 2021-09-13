#include <gui-engine/image.hpp>

#include <gui-engine/engine.hpp>

using namespace unibox::gui;

Image::Image(GuiEngine& engine, gui_resource_handle shader, gui_resource_handle tex, double posX, double posY, double width, double height)
    : GuiObject(engine, shader, 0, posX, posY, width, height) {
    this->tex = tex;
}

Image::~Image() {

}

void Image::render(double frameTime, double x, double y) {
    renderEngine.bind_texture_to_descriptor(getRenderObject(), "texture0", tex);
    GuiObject::render(frameTime, x, y);
}