#include <guis/tooltip.hpp>

#include <gui-engine/engine.hpp>
#include <util/global_resources.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace unibox;

gui::gui_resource_handle Tooltip::textureHandle = 0;

void Tooltip::ShaderInitFunc(gui::GuiEngine& engine, gui::gui_resource_handle handle) {
    engine.getRenderEngine().setShaderVariable(handle, "projectMatrix", engine.getProjectionMatrix());
}

void Tooltip::init(const std::string& font_name) {
    if(textureHandle == 0) textureHandle = guiEngine.createTexture("resources/gui/textures/tooltip_frame.png");

    this->visible = true;

    this->text = text;
    meshHandle = renderEngine.create_mesh();
    renderEngine.add_mesh_vertex_data(meshHandle, textObj.getMeshVec(), textObj.getVertexCount());

    GuiObject::setWidth(textSize*textObj.getMeshLength()+6*2);
    GuiObject::setHeight(textSize*textObj.getMeshHeight()+6*2+.15f);

    textObjRO = renderEngine.create_render_object(guiEngine.getOrCreateShader("font_shader", "shaders/gui/font/vertex.spv", "shaders/gui/font/fragment.spv", gui::SPIRV, ShaderInitFunc));
    renderEngine.attach_mesh(textObjRO, meshHandle);

    fontTex = *util::GlobalResources::getInstance()->get<gui::gui_resource_handle>(font_name + "_handle");
}

Tooltip::Tooltip(const std::string& font_name, gui::GuiEngine& engine, float textSize, const std::string& text) : 
    GuiObject(engine, engine.getOrCreateShader("tooltip_frame_shader", "shaders/gui/tooltip/vertex.spv", "shaders/gui/tooltip/fragment.spv", gui::SPIRV, ShaderInitFunc), 0, 100, 100, 64, 64),
    textObj(*util::GlobalResources::getInstance()->get<util::Font>(font_name), text, .15f) {
    this->textSize = textSize;
    position = std::nullopt;

    init(font_name);
}

Tooltip::Tooltip(const std::string& font_name, gui::GuiEngine& engine, float textSize, float x, float y, const std::string& text) :
    GuiObject(engine, engine.getOrCreateShader("tooltip_frame_shader", "shaders/gui/tooltip/vertex.spv", "shaders/gui/tooltip/fragment.spv", gui::SPIRV, ShaderInitFunc), 0, x, y, 128, 128),
    textObj(*util::GlobalResources::getInstance()->get<util::Font>(font_name), text, .15f) {
    this->textSize = textSize;
    position = std::optional(glm::vec2(x, y));

    init(font_name);
}

Tooltip::~Tooltip() {
    renderEngine.destroy_resource(meshHandle);
}

void Tooltip::setVisible(bool value) {
    this->visible = value;
}

void Tooltip::setText(const std::string& text) {
    textObj.setText(text);
    renderEngine.add_mesh_vertex_data(meshHandle, textObj.getMeshVec(), textObj.getVertexCount());

    GuiObject::setWidth(textSize*textObj.getMeshLength()+6*2);
    GuiObject::setHeight(textSize*textObj.getMeshHeight()+6*2+.15f);
}

void Tooltip::render(double frameTime, double x, double y) {
    if(visible) {
        auto pos = position.value_or(glm::vec2(x+textObj.getMeshLength()*textSize/2+12, y+textSize/2+9));
        GuiObject::setX(pos.x);
        GuiObject::setY(pos.y);

        renderEngine.bind_texture_to_descriptor(GuiObject::getRenderObject(), "texture0", textureHandle);
        GuiObject::render(frameTime, x, y);

        renderEngine.bind_texture_to_descriptor(textObjRO, "texture0", fontTex);
        int offset = (int)((2-textObj.getMeshHeight())*textSize/2);
        renderEngine.setShaderVariable(textObjRO, "transformMatrix", glm::scale(glm::translate(glm::mat4(1), glm::vec3(pos.x-textObj.getMeshLength()*textSize/2, pos.y+offset, 0)), glm::vec3(textSize, textSize, 1)));
        renderEngine.setShaderVariable(textObjRO, "color", glm::vec4(1, 1, 1, 1));
        renderEngine.render_object(textObjRO);
    }
}