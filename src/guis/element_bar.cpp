#include <guis/element_bar.hpp>

#include <gui-engine/engine.hpp>

#include <glm/gtc/matrix_transform.hpp>

using namespace unibox;

ElementBar::HotbarSlot::HotbarSlot(uint x, uint y, gui::GuiEngine& engine, gui::gui_resource_handle backgroundTexture, gui::gui_resource_handle iconTextureAtlas) : guiEngine(engine), renderEngine(engine.getRenderEngine()) {
    transform = glm::scale(glm::translate(glm::mat4(1), glm::vec3(x, y, 0)), glm::vec3(64, 64, 1));
    isSelected = false;
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
    font("resources/gui/fonts/Ubuntu-R.ttf", 64) {
    font.bakeAtlas();

    fontAtlas = GuiObject::renderEngine.create_texture(font.getAtlas().getWidth(), font.getAtlas().getHeight(), font.getAtlas().getAtlasData(), gui::NEAREST, gui::NEAREST);
    util::Text text = util::Text(font, "Test Text");
    textRO = GuiObject::renderEngine.create_render_object(engine.getShader("default_textured_shader"));
    gui::gui_resource_handle mesh = GuiObject::renderEngine.create_mesh();
    GuiObject::renderEngine.add_mesh_vertex_data(mesh, text.getMeshVec(), text.getVertexCount());
    GuiObject::renderEngine.attach_mesh(textRO, mesh);

    // Create the icon atlas.
    uint width, height;
    void* pixels = Particle::getIconImage(&width, &height);
    iconAtlas = GuiObject::renderEngine.create_texture(width, height, pixels, gui::NEAREST, gui::NEAREST);

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
}

ElementBar::~ElementBar() {
    GuiObject::renderEngine.destroy_resource(iconAtlas);
    for(int i = 0; i < 10; i++) delete hotbar[i];
}

void ElementBar::render(double frameTime, double x, double y) {
    for(int i = 0; i < 10; i++) {
        hotbar[i]->render();
    }

    GuiObject::renderEngine.setShaderVariable(textRO, "transformMatrix", glm::translate(glm::mat4(1), glm::vec3(guiEngine.getWidth()/2, guiEngine.getHeight()/2, 0)));
    GuiObject::renderEngine.bind_texture_to_descriptor(textRO, "texture0", fontAtlas);
    GuiObject::renderEngine.render_object(textRO);
}