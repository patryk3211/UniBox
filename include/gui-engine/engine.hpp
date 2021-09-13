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

    struct RenderEngine {
        uint width;
        uint height;

        std::function<gui_resource_handle(gui_resource_handle shader)> create_render_object;
        std::function<void(gui_resource_handle)> render_object;
        std::function<void(gui_resource_handle renderObject, gui_resource_handle mesh)> attach_mesh;
        std::function<void(gui_resource_handle renderObject, const std::string&, void*, size_t, size_t)> set_shader_variable;
        std::function<void(gui_resource_handle renderObject, const std::string&, gui_resource_handle, size_t offset, size_t length)> bind_buffer_to_descriptor;
        std::function<void(gui_resource_handle renderObject, const std::string&, gui_resource_handle)> bind_texture_to_descriptor;

        std::function<gui_resource_handle()> create_mesh;
        std::function<void(gui_resource_handle, const std::vector<uint8_t>& data, uint vertexCount)> add_mesh_vertex_data;
        std::function<void(gui_resource_handle, const std::vector<uint>& indices, uint vertexCount)> add_mesh_indices;
        std::function<gui_resource_handle(uint width, uint height, const void* data, ImageFilter minFilter, ImageFilter magFilter)> create_texture;

        std::function<gui_resource_handle(size_t)> create_buffer;
        std::function<void(gui_resource_handle, void*, size_t, size_t)> write_buffer;

        std::function<gui_resource_handle(const std::any& vertexCode, const std::any& fragmentCode, ShaderLanguage lang)> create_shader;

        std::function<void(gui_resource_handle)> destroy_resource;

        std::function<void(char* errorName, char* errorDescription)> error_callback;
    };

    class GuiEngine {
        RenderEngine renderEngine;
        gui_handle nextHandle;

        struct comparer;

        std::list<GuiObject*> guiObjects;
        std::unordered_map<gui_handle, GuiObject*> mappings;

        gui_resource_handle default_shader;
        gui_resource_handle default_mesh;

        uint width, height;
    public:
        GuiEngine(const RenderEngine& renderEngine);
        ~GuiEngine();

        gui_resource_handle createTexture(const std::string& filepath);

        gui_handle addItem(GuiObject* object);
        void removeItem(gui_handle handle);

        void render(double frameTime);

        void onMouseClick(double x, double y, int button);
        void onMouseHover(double x, double y);

        gui_resource_handle getDefaultMesh();
        gui_resource_handle getDefaultShader();

        const RenderEngine& getRenderEngine();

        uint getWidth();
        uint getHeight();
    };
}