#include <gui-engine/engine.hpp>

#include <istream>
#include <fstream>

#include <string.h>

using namespace unibox;

gui_resource_handle h;

GuiEngine::GuiEngine(const RenderEngine& renderEngine) {
    this->renderEngine = renderEngine;
    nextHandle = 1;

    std::ifstream vs("shaders/default/vertex.spv", std::ios::binary | std::ios::ate);
    std::ifstream fs("shaders/default/fragment.spv", std::ios::binary | std::ios::ate);
    if(!vs.is_open() || !fs.is_open()) return;

    size_t lenV = vs.tellg();
    size_t lenF = fs.tellg();

    vs.seekg(0);
    fs.seekg(0);

    std::vector<uint8_t> vcode(lenV);
    std::vector<uint8_t> fcode(lenF);

    vs.read((char*)vcode.data(), lenV);
    fs.read((char*)fcode.data(), lenF);

    gui_resource_handle sh = renderEngine.create_shader(vcode, fcode, SPIRV);

    float values[32];
    for(int i = 0; i < 32; i++) values[i] = 0;
    values[0] = 1;
    values[5] = 1;
    values[10] = 1;
    values[15] = 1;

    values[16] = 1;
    values[21] = 1;
    values[26] = 1;
    values[31] = 1;

    renderEngine.set_shader_variable(sh, "globalMatricies", values, 0, sizeof(values));

    gui_resource_handle msh = renderEngine.create_mesh();

    float data[3*4*4] = {
        0, 0, 0, 1,
        1, 1, 1, 1,

        0, 1, 0, 1,
        1, 1, 1, 1,

        1, 0, 0, 1,
        1, 1, 1, 1
    };

    std::vector<uint8_t> vec(sizeof(data));
    memcpy(vec.data(), data, sizeof(data));
    renderEngine.add_mesh_vertex_data(msh, vec, 3);

    gui_resource_handle ren = renderEngine.create_render_object();
    renderEngine.attach_mesh(ren, msh);
    renderEngine.set_render_object_shader(ren, sh);
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