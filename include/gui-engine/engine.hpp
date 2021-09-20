#pragma once

#include <algorithm>
#include <any>
#include <map>
#include <string>
#include <list>

#include <gui-engine/object.hpp>

namespace unibox::gui {
    enum ShaderLanguage {
        GLSL,
        SPIRV
    };

    enum ImageFilter {
        NEAREST,
        LINEAR
    };

    enum ImageFormat {
        R8G8B8A8,
        R8G8B8,
        R8G8,
        R8
    };

    struct RenderEngine {
        uint width;
        uint height;

        std::function<gui_resource_handle(gui_resource_handle shader)> create_render_object;
        std::function<void(gui_resource_handle)> render_object;
        std::function<void(gui_resource_handle renderObject, gui_resource_handle mesh)> attach_mesh;
        std::function<void(gui_resource_handle renderObject, const std::string&, const void*, size_t, size_t)> set_shader_variable;
        std::function<void(gui_resource_handle renderObject, const std::string&, gui_resource_handle, size_t offset, size_t length)> bind_buffer_to_descriptor;
        std::function<void(gui_resource_handle renderObject, const std::string&, gui_resource_handle)> bind_texture_to_descriptor;

        std::function<gui_resource_handle()> create_mesh;
        std::function<void(gui_resource_handle, const std::vector<uint8_t>& data, uint vertexCount)> add_mesh_vertex_data;
        std::function<void(gui_resource_handle, const std::vector<uint>& indices, uint vertexCount)> add_mesh_indices;
        std::function<gui_resource_handle(uint width, uint height, const void* data, ImageFormat format, ImageFilter minFilter, ImageFilter magFilter)> create_texture;

        std::function<gui_resource_handle(size_t)> create_buffer;
        std::function<void(gui_resource_handle, void*, size_t, size_t)> write_buffer;

        std::function<gui_resource_handle(const std::any& vertexCode, const std::any& fragmentCode, ShaderLanguage lang)> create_shader;

        std::function<void(gui_resource_handle)> destroy_resource;

        std::function<void(char* errorName, char* errorDescription)> error_callback;

        // TODO: Implement more function templates and stuff.
        void setShaderVariable(gui_resource_handle resource, const std::string& variableName, const void* value, size_t offset, size_t length) const { set_shader_variable(resource, variableName, value, offset, length); }
        template<typename T> void setShaderVariable(gui_resource_handle resource, const std::string& variableName, const T& value) const {
            set_shader_variable(resource, variableName, &value, 0, sizeof(value));
        }
    };

    class GuiEngine {
        RenderEngine renderEngine;
        gui_handle nextHandle;

        struct comparer;

        std::list<GuiObject*> guiObjects;
        std::unordered_map<gui_handle, GuiObject*> mappings;

        std::unordered_map<std::string, gui_resource_handle> shaders;

        gui_resource_handle default_mesh;

        uint width, height;

        glm::mat4 projection;
    public:
        GuiEngine(const RenderEngine& renderEngine);
        ~GuiEngine();

        gui_resource_handle createTexture(const std::string& filepath);

        gui_resource_handle createShader(const std::string& vertex, const std::string& fragment, ShaderLanguage lang, const std::string& registryName);
        gui_resource_handle getShader(const std::string& shaderName);
        gui_resource_handle getOrCreateShader(const std::string& shaderName, const std::string& vertex, const std::string& fragment, ShaderLanguage lang, const std::function<void(GuiEngine&, gui_resource_handle)>& initFunc);

        gui_handle addItem(GuiObject* object);
        void removeItem(gui_handle handle);

        void render(double frameTime, double x, double y);

        void onMouseDown(double x, double y, int button);
        void onMouseUp(double x, double y, int button);

        gui_resource_handle getDefaultMesh();

        const RenderEngine& getRenderEngine();

        uint getWidth();
        uint getHeight();

        const glm::mat4& getProjectionMatrix();
    };
}