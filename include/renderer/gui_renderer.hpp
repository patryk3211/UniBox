#pragma once

#include <gui-engine/engine.hpp>
#include <vk-engine/engine.hpp>
#include <vk-engine/buffer.hpp>
#include <vk-engine/image.hpp>
#include <vk-engine/window.hpp>

#include <unordered_map>
#include <queue>
#include <list>

namespace unibox {
    class GuiRenderer {
        static GuiRenderer* instance;

        Window& window;

        gui::RenderEngine functions;

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

        struct Texture {
            Image* image;
        };

        struct GuiShader {
            struct DescriptorInfo {
                uint set;
                uint binding;
                size_t size;
                VkDescriptorType type;
                gui::gui_resource_handle boundBuffer;
                size_t boundOffset;
                size_t boundLength;
                bool isDefault;
            };

            struct DescriptorMemberInfo {
                DescriptorInfo* descriptor;
                size_t offset;
            };

            struct BufferCreateInfo {
                uint binding;
                size_t size;
                DescriptorInfo* info;
            };

            GraphicsPipeline* pipeline;
            std::list<BufferCreateInfo> set0BufferCreate;
            std::unordered_map<std::string, DescriptorMemberInfo> descriptorMembers;
            std::unordered_map<std::string, DescriptorInfo*> descriptors;
            std::unordered_map<std::string, uint> pushConstants;
            uint8_t* pushConstant;
            uint pushSizeVertex;
            uint pushSizeFragment;
            VkDescriptorSet shaderSet;
        };

        struct RenderObject {
            Mesh* mesh;
            GuiShader* shader;
            VkDescriptorSet objectSet;
        };

        struct GuiBuffer {
            Buffer* buffer;
        };

        struct RenderingState {
            GuiShader*& currentShader;
        };

        const std::type_info& RENDER_OBJECT = typeid(RenderObject);
        const std::type_info& MESH = typeid(Mesh);
        const std::type_info& SHADER = typeid(GuiShader);
        const std::type_info& BUFFER = typeid(GuiBuffer);
        const std::type_info& TEXTURE = typeid(Texture);

        std::list<std::function<void(double, double, double)>> renderCallbacks;

        gui::gui_resource_handle nextHandle;
        std::unordered_map<gui::gui_resource_handle, Resource*> resources;

        std::queue<std::function<void(RenderingState&, VkCommandBuffer cmd)>> renderActions;

        template<typename T> std::optional<T*> getResource(gui::gui_resource_handle handle) {
            auto resource = resources.find(handle);
            if(resource == resources.end() || resource->second->type() != typeid(T)) return std::nullopt;
            std::optional<T*> opt = resource->second->get<T>();
            if(opt.has_value()) return std::optional(opt.value());
            return std::nullopt;
        }
    public:
        GuiRenderer(uint width, uint height, Window& window);
        ~GuiRenderer();

        void render(VkCommandBuffer cmd);

        const gui::RenderEngine& getRenderEngineFunctions();
        void addRenderCallback(std::function<void(double, double, double)> callback);

        static GuiRenderer* getInstance();
    };
}