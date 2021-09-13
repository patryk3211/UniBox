#include <gui-engine/engine.hpp>

#include <istream>
#include <fstream>
#include <string.h>

#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb/stb_image.h>

using namespace unibox::gui;

#define DEFAULT_VERTEX_SHADER "shaders/gui/default/vertex.spv"
#define DEFAULT_FRAGMENT_SHADER "shaders/gui/default/fragment.spv"

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

    glm::mat4 projection = glm::ortho(0.0f, (float)renderEngine.width, 0.0f, (float)renderEngine.height, 10.0f, -10.0f);
    renderEngine.set_shader_variable(default_shader, "matrices", &projection, 0, sizeof(projection));

    default_mesh = renderEngine.create_mesh();

    float data[] = {
        -0.5, -0.5, 0.0,  0.0, 0.0,
        -0.5,  0.5, 0.0,  0.0, 1.0,
         0.5, -0.5, 0.0,  1.0, 0.0,
        -0.5,  0.5, 0.0,  0.0, 1.0,
         0.5,  0.5, 0.0,  1.0, 1.0,
         0.5, -0.5, 0.0,  1.0, 0.0
    };

    std::vector<uint8_t> vec(sizeof(data));
    memcpy(vec.data(), data, sizeof(data));
    renderEngine.add_mesh_vertex_data(default_mesh, vec, 6);

    this->width = renderEngine.width;
    this->height = renderEngine.height;
}

GuiEngine::~GuiEngine() {

}

uint GuiEngine::getWidth() {
    return width;
}

uint GuiEngine::getHeight() {
    return height;
}

gui_handle GuiEngine::addItem(GuiObject* object) {
    // TODO: [11.09.2021] Should propably check if the handle is actually available.
    auto last = guiObjects.begin();
    gui_handle handle = nextHandle;
    nextHandle++;
    for(auto iter = guiObjects.begin(); iter != guiObjects.end(); iter++) {
        if((*iter)->getLayer() < object->getLayer()) {
            auto ins = guiObjects.insert(iter, object);
            mappings.insert({ handle, object });
            return handle;
        }
        last = iter;
    }
    guiObjects.push_back(object);
    mappings.insert({ handle, object });
    return handle;
}

void GuiEngine::removeItem(gui_handle handle) {
    auto map = mappings.find(handle);
    if(map != mappings.end()) {
        guiObjects.remove(map->second);
        mappings.erase(handle);
    }
}

void GuiEngine::render(double frameTime) {
    std::for_each(guiObjects.rbegin(), guiObjects.rend(), [frameTime](GuiObject* object) {
        object->render(frameTime);
    });
}

gui_resource_handle GuiEngine::createTexture(const std::string& filepath) {
    int width, height, channels;
    stbi_uc* pixels = stbi_load(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    return renderEngine.create_texture(width, height, pixels, LINEAR, NEAREST);
}

void GuiEngine::onMouseClick(double x, double y, int button) {
    for(auto& object : guiObjects) {
        if(x >= object->getX() && y >= object->getY() && x < object->getX()+object->getWidth() && y < object->getY()+object->getHeight()) {
            object->mouseClick(x, y, button);
            break;
        }
    }
}

void GuiEngine::onMouseHover(double x, double y) {
    for(auto& object : guiObjects) {
        if(x >= object->getX() && y >= object->getY() && x < object->getX()+object->getWidth() && y < object->getY()+object->getHeight()) {
            object->mouseHover(x, y);
            break;
        }
    }
}

const RenderEngine& GuiEngine::getRenderEngine() {
    return renderEngine;
}

gui_resource_handle GuiEngine::getDefaultMesh() {
    return default_mesh;
}

gui_resource_handle GuiEngine::getDefaultShader() {
    return default_shader;
}