#include <guis/element_bar.hpp>

#include <gui-engine/engine.hpp>

using namespace unibox;

ElementBar::ElementBar(gui::GuiEngine& engine) : 
    GuiObject(engine, engine.getShader("default_colored_shader"), 0,
        engine.getWidth()/2, engine.getHeight()/2,
        engine.getWidth()/2, engine.getHeight()/1.2) {
    
}

ElementBar::~ElementBar() {

}

void ElementBar::render(double frameTime, double x, double y) {
    glm::vec4 color = glm::vec4(0.1, 0.1, 0.1, 0.5);
    renderEngine.set_shader_variable(getRenderObject(), "color", &color, 0, sizeof(glm::vec4));
    GuiObject::render(frameTime, x, y);
}