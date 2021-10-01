#include <gui-engine/engine.hpp>

#include <istream>
#include <fstream>
#include <string.h>

#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb/stb_image.h>

using namespace unibox::gui;

GuiEngine::GuiEngine(const RenderEngine& renderEngine) {
    this->renderEngine = renderEngine;
    nextHandle = 1;

    gui_resource_handle shader = createShader("shaders/gui/texture/vertex.spv", "shaders/gui/texture/fragment.spv", SPIRV, "default_textured_shader");
    gui_resource_handle shader2 = createShader("shaders/gui/color/vertex.spv", "shaders/gui/color/fragment.spv", SPIRV, "default_colored_shader");
    gui_resource_handle shader3 = createShader("shaders/gui/atlas/vertex.spv", "shaders/gui/atlas/fragment.spv", SPIRV, "default_texture_atlas_shader");

    projection = glm::ortho(0.0f, (float)renderEngine.width, 0.0f, (float)renderEngine.height, 10.0f, -10.0f);
    renderEngine.set_shader_variable(shader, "projectMatrix", &projection, 0, sizeof(projection));
    renderEngine.set_shader_variable(shader2, "projectMatrix", &projection, 0, sizeof(projection));
    renderEngine.set_shader_variable(shader3, "projectMatrix", &projection, 0, sizeof(projection));

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

gui_resource_handle GuiEngine::createShader(const std::string& vertex, const std::string& fragment, ShaderLanguage lang, const std::string& registryName) {
    std::ifstream vs(vertex, std::ios::binary | std::ios::ate);
    std::ifstream fs(fragment, std::ios::binary | std::ios::ate);
    if(!vs.is_open() || !fs.is_open()) return 0;

    size_t lenV = vs.tellg();
    size_t lenF = fs.tellg();

    vs.seekg(0);
    fs.seekg(0);

    std::vector<uint8_t> vcode(lenV);
    std::vector<uint8_t> fcode(lenF);

    vs.read((char*)vcode.data(), lenV);
    fs.read((char*)fcode.data(), lenF);

    gui_resource_handle handle = renderEngine.create_shader(vcode, fcode, lang);
    shaders.insert({ registryName, handle });
    return handle;
}

gui_resource_handle GuiEngine::getShader(const std::string& shaderName) {
    auto res = shaders.find(shaderName);
    if(res == shaders.end()) return 0;
    return res->second;
}

gui_resource_handle GuiEngine::getOrCreateShader(const std::string& shaderName, const std::string& vertex, const std::string& fragment, ShaderLanguage lang, const std::function<void(GuiEngine&, gui_resource_handle)>& initFunc) {
    gui_resource_handle handle = getShader(shaderName);
    if(handle == 0) {
        handle = createShader(vertex, fragment, lang, shaderName);
        initFunc(*this, handle);
    }
    return handle;
}

void GuiEngine::render(double frameTime, double x, double y) {
    std::for_each(guiObjects.rbegin(), guiObjects.rend(), [frameTime, x, y](GuiObject* object) {
        object->render(frameTime, x, y);
    });
}

gui_resource_handle GuiEngine::createTexture(const std::string& filepath) {
    int width, height, channels;
    stbi_uc* pixels = stbi_load(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    return renderEngine.create_texture(width, height, pixels, R8G8B8A8, NEAREST, NEAREST);
}

void GuiEngine::onMouseDown(double x, double y, int button) {
    for(auto& object : guiObjects) {
        if(object->isInside(x, y) && object->mouseDown(x, y, button)) break;
    }
}

void GuiEngine::onMouseUp(double x, double y, int button) {
    for(auto& object : guiObjects) {
        if(object->isInside(x, y) && object->mouseUp(x, y, button)) break;
    }
}

const RenderEngine& GuiEngine::getRenderEngine() {
    return renderEngine;
}

gui_resource_handle GuiEngine::getDefaultMesh() {
    return default_mesh;
}

const glm::mat4& GuiEngine::getProjectionMatrix() {
    return projection;
}
