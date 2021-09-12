#include <gui-engine/engine.hpp>

#include <istream>
#include <fstream>
#include <string.h>

#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace unibox;

#define DEFAULT_VERTEX_SHADER "shaders/gui/default/vertex.spv"
#define DEFAULT_FRAGMENT_SHADER "shaders/gui/default/fragment.spv"
gui_resource_handle h;
GuiEngine::GuiEngine(const RenderEngine& renderEngine) {
    this->renderEngine = renderEngine;
    nextHandle = 1;

    std::ifstream vs(DEFAULT_VERTEX_SHADER, std::ios::binary | std::ios::ate);
    std::ifstream fs(DEFAULT_FRAGMENT_SHADER, std::ios::binary | std::ios::ate);
    if(!vs.is_open() || !fs.is_open()) return;

    size_t lenV = vs.tellg();
    size_t lenF = fs.tellg();

    vs.seekg(0);
    fs.seekg(0);

    std::vector<uint8_t> vcode(lenV);
    std::vector<uint8_t> fcode(lenF);

    vs.read((char*)vcode.data(), lenV);
    fs.read((char*)fcode.data(), lenF);

    default_shader = renderEngine.create_shader(vcode, fcode, SPIRV);

    float aspect = 1280.0/720.0;
    glm::mat4 projection = glm::ortho(-aspect, aspect, -1.0f, 1.0f, 10.0f, -10.0f);
    projection[1][0] = -projection[1][0];
    projection[1][1] = -projection[1][1];
    projection[1][2] = -projection[1][2];
    projection[1][3] = -projection[1][3];

    glm::mat4 transform = glm::translate(glm::mat4(1), glm::vec3(0, 0, 0));

    renderEngine.set_shader_variable(default_shader, "matrices", &projection, 0, sizeof(projection));
    renderEngine.set_shader_variable(default_shader, "transformMatrix", &transform, 0, sizeof(transform));

    default_mesh = renderEngine.create_mesh();

    float data[] = {
        -0.5, -0.5, 0.0,
        -0.5,  0.5, 0.0,
         0.5, -0.5, 0.0,
        -0.5,  0.5, 0.0,
         0.5,  0.5, 0.0,
         0.5, -0.5, 0.0
    };

    std::vector<uint8_t> vec(sizeof(data));
    memcpy(vec.data(), data, sizeof(data));
    renderEngine.add_mesh_vertex_data(default_mesh, vec, 6);

    gui_resource_handle ren = renderEngine.create_render_object();
    renderEngine.attach_mesh(ren, default_mesh);
    renderEngine.set_render_object_shader(ren, default_shader);

    h = ren;
}

GuiEngine::~GuiEngine() {

}

gui_handle GuiEngine::addItem(const GuiObject& object) {
    // TODO: [11.09.2021] Should propably check if the handle is actually available.
    guiObjects.insert( { nextHandle, object } );
    gui_handle handle = nextHandle;
    nextHandle++;
    return handle;
}

void GuiEngine::removeItem(gui_handle handle) {
    guiObjects.erase(handle);
}

void GuiEngine::render(double frameTime) {
    for(auto& [handle, object] : guiObjects) object.render(frameTime);
    renderEngine.render_object(h);
}

gui_resource_handle GuiEngine::createRenderObject() {
    return renderEngine.create_render_object();
}

void GuiEngine::destroyRenderObject(gui_resource_handle handle) {
    renderEngine.destroy_resource(handle);
}

void GuiEngine::onMouseClick(double x, double y, int button) {
    for(auto& [handle, object] : guiObjects) {
        if(x >= object.getX() && y >= object.getY() && x < object.getX()+object.getWidth() && y < object.getY()+object.getHeight()) {
            object.mouseClick(x, y, button);
            break;
        }
    }
}

void GuiEngine::onMouseHover(double x, double y) {
    for(auto& [handle, object] : guiObjects) {
        if(x >= object.getX() && y >= object.getY() && x < object.getX()+object.getWidth() && y < object.getY()+object.getHeight()) {
            object.mouseHover(x, y);
            break;
        }
    }
}