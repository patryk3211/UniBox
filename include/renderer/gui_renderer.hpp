#pragma once

#include <gui-engine/engine.hpp>
#include <vk-engine/engine.hpp>
#include <vk-engine/buffer.hpp>

#include <unordered_map>
#include <queue>
#include <list>

namespace unibox {
    class GuiRenderer {
        static GuiRenderer* instance;

        RenderEngine functions;

        struct Resource {
            const std::type_info& type_info;
            std::any value;
            std::optional<std::function<void(Resource&)>> destroyCall;

            template<typename T> Resource() : type_info(typeid(T)) {
                value.reset();
                destroyCall = std::nullopt;
            }

            template<typename T> Resource(const T& value) : type_info(typeid(T)) { 
                this->value = value;
                destroyCall = std::nullopt;
            }

            template<typename T> Resource(const T& value, const std::function<void(Resource&)>& destroyCall) : type_info(typeid(T)) {
                this->value = value;
                this->destroyCall = destroyCall;
            }

            template<typename T> std::optional<T*> get() {
                if(typeid(T) == type_info) return std::optional(&std::any_cast<T&>(value));
                return std::nullopt;
            }

            ~Resource() {
                if(destroyCall.has_value()) destroyCall.value()(*this);
            }

            const std::type_info& type() {
                return type_info;
            }
        };

        struct Mesh {
            Buffer* vertexBuffer;
            Buffer* indexBuffer;

            uint vertexCount;
        };

        struct GuiShader {
            struct DescriptorInfo {
                uint set;
                uint binding;
                VkDescriptorType type;
                gui_resource_handle boundBuffer;
                size_t boundOffset;
                size_t boundLength;
                bool isDefault;
            };

            GraphicsPipeline* pipeline;
            std::unordered_map<std::string, DescriptorInfo> descriptors;
            std::unordered_map<std::string, uint> pushConstants;
            uint8_t* pushConstant;
            uint pushSizeVertex;
            uint pushSizeFragment;
        };

        struct RenderObject {
            Mesh* mesh;
            GuiShader* shader;
            int layer;
        };

        struct GuiBuffer {
            Buffer* buffer;
        };

        const std::type_info& RENDER_OBJECT = typeid(RenderObject);
        const std::type_info& MESH = typeid(Mesh);
        const std::type_info& SHADER = typeid(GuiShader);
        const std::type_info& BUFFER = typeid(GuiBuffer);

        std::list<std::function<void(double)>> renderCallbacks;

        gui_resource_handle nextHandle;
        std::unordered_map<gui_resource_handle, Resource*> resources;

        std::list<RenderObject*> objectsToRender;

        template<typename T> std::optional<T*> getResource(gui_resource_handle handle) {
            auto resource = resources.find(handle);
            if(resource == resources.end() || resource->second->type() != typeid(T)) return std::nullopt;
            std::optional<T*> opt = resource->second->get<T>();
            if(opt.has_value()) return std::optional(opt.value());
            return std::nullopt;
        }
    public:
        GuiRenderer();
        ~GuiRenderer();

        void render(VkCommandBuffer cmd);

        const RenderEngine& getRenderEngineFunctions();
        void addRenderCallback(std::function<void(double)> callback);

        static GuiRenderer* getInstance();
    };
}