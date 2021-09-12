#pragma once

#include <algorithm>
#include <any>
#include <map>
#include <string>

#include <gui-engine/object.hpp>

namespace unibox {
    enum ShaderLanguage {
        GLSL,
        SPIRV
    };

    struct RenderEngine {
        std::function<gui_resource_handle()> create_render_object;
        std::function<void(gui_resource_handle)> render_object;
        std::function<void(gui_resource_handle renderObject, gui_resource_handle shader)> set_render_object_shader;
        std::function<void(gui_resource_handle renderObject, gui_resource_handle mesh)> attach_mesh;
        std::function<void(gui_resource_handle, int)> set_render_layer;

        std::function<gui_resource_handle()> create_mesh;
        std::function<void(gui_resource_handle, const std::vector<uint8_t>& data, uint vertexCount)> add_mesh_vertex_data;
        std::function<void(gui_resource_handle, const std::vector<uint>& indices, uint vertexCount)> add_mesh_indices;
        std::function<gui_resource_handle(uint width, uint height, const void* data)> create_texture;

        std::function<gui_resource_handle(size_t)> create_buffer;
        std::function<void(gui_resource_handle, void*, size_t, size_t)> write_buffer;

        std::function<gui_resource_handle(const std::any& vertexCode, const std::any& fragmentCode, ShaderLanguage lang)> create_shader;
        std::function<void(gui_resource_handle, const std::string&, gui_resource_handle, size_t offset, size_t length)> bind_buffer_to_descriptor;
        std::function<void(gui_resource_handle, const std::string&, void*, size_t, size_t)> set_shader_variable;

        std::function<void(gui_resource_handle)> destroy_resource;

        std::function<void(char* errorName, char* errorDescription)> error_callback;
    };

    class GuiEngine {
        RenderEngine renderEngine;
        gui_handle nextHandle;

        struct comparer;

        std::unordered_map<gui_handle, GuiObject> guiObjects;
    public:
        GuiEngine(const RenderEngine& renderEngine);
        ~GuiEngine();

        gui_handle addItem(const GuiObject& object);
        void removeItem(gui_handle handle);

        void render(double frameTime);

        void onMouseClick(double x, double y, int button);
        void onMouseHover(double x, double y);

        gui_resource_handle createRenderObject();
        void destroyRenderObject(gui_resource_handle handle);
    };
}