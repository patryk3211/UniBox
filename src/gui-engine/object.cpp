#include <gui-engine/object.hpp>

#include <gui-engine/engine.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace unibox::gui;

GuiObject::GuiObject(GuiEngine& engine, int layer, double posX, double posY, double scaleX, double scaleY) : guiEngine(engine), renderEngine(engine.getRenderEngine()) {
    this->layer = layer;

    this->posX = posX;
    this->posY = posY;

    this->scaleX = scaleX;
    this->scaleY = scaleY;

    this->mesh = engine.getDefaultMesh();
    this->shader = engine.getDefaultShader();

    renderObjectHandle = renderEngine.create_render_object();
    renderEngine.attach_mesh(renderObjectHandle, mesh);
    renderEngine.set_render_object_shader(renderObjectHandle, shader);

    handle = engine.addItem(this);

    rebuildMatrix();
}

GuiObject::~GuiObject() {
    guiEngine.removeItem(handle);
    renderEngine.destroy_resource(renderObjectHandle);
}

gui_resource_handle GuiObject::getShader() {
    return shader;
}

gui_resource_handle GuiObject::getMesh() {
    return mesh;
}

void GuiObject::setShader(gui_resource_handle shader) {
    this->shader = shader;
    renderEngine.set_render_object_shader(renderObjectHandle, shader);
}

void GuiObject::setMesh(gui_resource_handle mesh) {
    this->mesh = mesh;
    renderEngine.attach_mesh(renderObjectHandle, mesh);
}

int GuiObject::getLayer() {
    return layer;
}

void GuiObject::setLayer(int layer) {
    this->layer = layer;
}

void GuiObject::render(double frameTime) {
    renderEngine.set_shader_variable(shader, "transformMatrix", &transformMatrix, 0, sizeof(glm::mat4));
    renderEngine.render_object(renderObjectHandle);
}

void GuiObject::mouseHover(double x, double y) { }
void GuiObject::mouseClick(double x, double y, int button) { }

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

void GuiObject::setX(double value) {
    posX = value;
    rebuildMatrix();
}

void GuiObject::setY(double value) {
    posY = value;
    rebuildMatrix();
}

void GuiObject::setWidth(double value) {
    scaleX = value;
    rebuildMatrix();
}

void GuiObject::setHeight(double value) {
    scaleY = value;
    rebuildMatrix();
}

void GuiObject::rebuildMatrix() {
    transformMatrix = glm::scale(glm::translate(glm::mat4(1), glm::vec3(posX, posY, 0)), glm::vec3(scaleX, scaleY, 1));
}

bool GuiObject::operator==(const GuiObject& other) {
    return handle == other.handle;
}
