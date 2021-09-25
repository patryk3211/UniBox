#include <guis/element_bar.hpp>

#include <gui-engine/engine.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace unibox;

ElementBar::HotbarSlot::HotbarSlot(uint x, uint y, gui::GuiEngine& engine, gui::gui_resource_handle backgroundTexture, gui::gui_resource_handle iconTextureAtlas) : guiEngine(engine), renderEngine(engine.getRenderEngine()) {
    this->x = x;
    this->y = y;

    transform = glm::scale(glm::translate(glm::mat4(1), glm::vec3(x, y, 0)), glm::vec3(64, 64, 1));
    // Create render objects
    background_RO = renderEngine.create_render_object(guiEngine.getShader("default_textured_shader"));
    particle_icon_RO = renderEngine.create_render_object(guiEngine.getShader("default_texture_atlas_shader"));

    // Bind meshes
    renderEngine.attach_mesh(background_RO, guiEngine.getDefaultMesh());
    renderEngine.attach_mesh(particle_icon_RO, guiEngine.getDefaultMesh());

    this->background_TEX = backgroundTexture;
    this->icon_TEX = iconTextureAtlas;

    particle = 0;
}

ElementBar::HotbarSlot::~HotbarSlot() {
    renderEngine.destroy_resource(background_RO);
}

bool ElementBar::HotbarSlot::isInside(double x, double y) {
    return x >= this->x-24 && y >= this->y-24 && x < this->x+24 && y < this->y+24;
}

void ElementBar::HotbarSlot::render() {
    renderEngine.bind_texture_to_descriptor(background_RO, "texture0", background_TEX);
    renderEngine.setShaderVariable(background_RO, "tint", glm::vec4(1, 1, 1, 0.95));
    renderEngine.setShaderVariable(background_RO, "transformMatrix", transform);
    renderEngine.render_object(background_RO);
    if(particle != 0) {
        renderEngine.bind_texture_to_descriptor(particle_icon_RO, "texture0", icon_TEX);
        renderEngine.setShaderVariable(particle_icon_RO, "transformMatrix", glm::scale(transform, glm::vec3(0.75, 0.75, 1)));
        auto& iconCoords = particle->getTextureAtlasIcon();
        renderEngine.setShaderVariable(particle_icon_RO, "uv_offset", glm::vec2(iconCoords.x, iconCoords.y));
        renderEngine.setShaderVariable(particle_icon_RO, "uv_size", glm::vec2(iconCoords.width, iconCoords.height));
        renderEngine.render_object(particle_icon_RO);
    }
}

ElementBar::ElementBar(gui::GuiEngine& engine) : 
    GuiObject(engine, engine.getShader("default_colored_shader"), 0,
        engine.getWidth()/2, engine.getHeight()/2,
        engine.getWidth(), engine.getHeight()),/*2, 1.2*/
    slotTexture(engine.createTexture("resources/gui/textures/element_slot.png")),
    tooltip("main_font", engine, 8.0f, "tooltip") {
    tooltip.removeFromAutoRender();

    // Create the icon atlas.
    uint width, height;
    void* pixels = Particle::getIconImage(&width, &height);
    iconAtlas = GuiObject::renderEngine.create_texture(width, height, pixels, gui::R8G8B8A8, gui::NEAREST, gui::NEAREST);

    { // Create the hotbar
        uint hotbarStart = guiEngine.getWidth()/2-(10*60/2);
        for(int i = 0; i < 10; i++) {
            hotbar[i] = new HotbarSlot(hotbarStart+i*60, guiEngine.getHeight()-32, guiEngine, slotTexture, iconAtlas);
        }
        hotbar[0]->particle = &Particle::getParticle("unibox:copper");
        hotbar[1]->particle = &Particle::getParticle("unibox:nsilicon");
        hotbar[2]->particle = &Particle::getParticle("unibox:psilicon");
        hotbar[3]->particle = &Particle::getParticle("unibox:gate");
        hotbar[4]->particle = &Particle::getParticle("unibox:spark");
    }
    setSelect(0);

    // Create the selected slot highlight.
    selectionRO = GuiObject::renderEngine.create_render_object(engine.getShader("default_colored_shader"));
    GuiObject::renderEngine.attach_mesh(selectionRO, engine.getDefaultMesh());
    GuiObject::renderEngine.setShaderVariable(selectionRO, "color", glm::vec4(0.5, 0.5, 0.5, 0.05));
}

ElementBar::~ElementBar() {
    GuiObject::renderEngine.destroy_resource(iconAtlas);
    GuiObject::renderEngine.destroy_resource(slotTexture);
    GuiObject::renderEngine.destroy_resource(selectionRO);
    for(int i = 0; i < 10; i++) delete hotbar[i];
}

void ElementBar::setSelect(uint index) {
    this->selected = index;
    highlightMatrix = glm::scale(glm::translate(glm::mat4(1), 
    glm::vec3(hotbar[selected]->x, hotbar[selected]->y, 0)), glm::vec3(64, 64, 1));
}

void ElementBar::render(double frameTime, double x, double y) {
    bool found = false;
    // Render the hotbar.
    for(int i = 0; i < 10; i++) {
        hotbar[i]->render();
        if(!found && hotbar[i]->particle != 0 && hotbar[i]->isInside(x, y)) {
            found = true;
            tooltip.setText(hotbar[i]->particle->getDisplayName());
            tooltip.setVisible(true);
        }
    }
    { // Render the highlight.
        GuiObject::renderEngine.setShaderVariable(selectionRO, "transformMatrix", highlightMatrix);
        GuiObject::renderEngine.render_object(selectionRO);
    }

    // Perhaps render the tooltip too.
    if(!found) tooltip.setVisible(false);
    tooltip.render(frameTime, x, y);
}

void ElementBar::mouseDown(double x, double y, int button) {
    for(int i = 0; i < 10; i++) {
        if(hotbar[i]->isInside(x, y)) {
            setSelect(i);
            break;
        }
    }
}
