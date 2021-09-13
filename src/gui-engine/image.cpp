#include <gui-engine/image.hpp>

#include <gui-engine/engine.hpp>

using namespace unibox::gui;

Image::Image(GuiEngine& engine, gui_resource_handle tex, double posX, double posY, double width, double height)
    : GuiObject(engine, 0, posX, posY, width, height) {
    this->tex = tex;
}

Image::~Image() {

}

void Image::render(double frameTime) {
    renderEngine.bind_texture_to_descriptor(getShader(), "texture0", tex);
    GuiObject::render(frameTime);
}