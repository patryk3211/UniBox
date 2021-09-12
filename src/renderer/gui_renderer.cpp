#include <renderer/gui_renderer.hpp>

#include <spirv/spirv_reflect.h>
#include <util/format.hpp>

#include <spdlog/spdlog.h>

using namespace unibox;

GuiRenderer* GuiRenderer::instance = 0;

GuiRenderer::GuiRenderer() {
    nextHandle = 1;

    functions.create_render_object = [&]() {
        RenderObject object = {};


        Resource* resource = new Resource(object);
        gui_resource_handle handle = nextHandle;
        resources.insert({ handle, resource });
        nextHandle++;

        return handle;
    };
    functions.destroy_resource = [&](gui_resource_handle handle) { 
        if(handle != 0 && resources.find(handle) != resources.end()) {
            delete resources[handle];
            resources.erase(handle);
        }
    };

    functions.render_object = [&](gui_resource_handle handle) {
        std::optional<RenderObject*> obj = getResource<RenderObject>(handle);
        if(obj.has_value()) {
            if(obj.value()->pipeline == 0) return;
            for(auto iter = objectsToRender.begin(); iter != objectsToRender.end(); iter++) {
                if((*iter)->layer < obj.value()->layer) {
                    objectsToRender.insert(iter, &std::any_cast<RenderObject&>(resources[handle]->value));
                    return;
                }
            }
            objectsToRender.push_back(&std::any_cast<RenderObject&>(resources[handle]->value));
        }
    };

    functions.create_shader = [&](const std::any& vertex, const std::any& fragment, ShaderLanguage lang) {
        if(lang != SPIRV) return (gui_resource_handle)0; // Don't handle other languages for now.

        Shader vert = Shader(VK_SHADER_STAGE_VERTEX_BIT, "main");
        const void* vertCode;
        size_t vertSize;
        if(vertex.type() == typeid(std::vector<uint32_t>)) {
            const std::vector<uint32_t>& code = std::any_cast<const std::vector<uint32_t>&>(vertex);
            vert.addCode(code.size()*4, code.data());
            vertCode = code.data();
            vertSize = code.size()*4;
        } else if(vertex.type() == typeid(std::vector<uint8_t>)) {
            const std::vector<uint8_t>& code = std::any_cast<const std::vector<uint8_t>&>(vertex);
            vert.addCode(code.size(), (uint32_t*)code.data());
            vertCode = code.data();
            vertSize = code.size();
        } else return (gui_resource_handle)0;

        Shader frag = Shader(VK_SHADER_STAGE_FRAGMENT_BIT, "main");
        const void* fragCode;
        size_t fragSize;
        if(fragment.type() == typeid(std::vector<uint32_t>)) {
            const std::vector<uint32_t>& code = std::any_cast<const std::vector<uint32_t>&>(fragment);
            frag.addCode(code.size()*4, code.data());
            fragCode = code.data();
            fragSize = code.size()*4;
        } else if(fragment.type() == typeid(std::vector<uint8_t>)) {
            const std::vector<uint8_t>& code = std::any_cast<const std::vector<uint8_t>&>(fragment);
            frag.addCode(code.size(), (uint32_t*)code.data());
            fragCode = code.data();
            fragSize = code.size();
        } else return (gui_resource_handle)0;

        GraphicsPipeline* pipeline = new GraphicsPipeline();
        pipeline->addShader(&vert);
        pipeline->addShader(&frag);

        pipeline->enableAlphaBlend();

        spv_reflect::ShaderModule vertReflectMod = spv_reflect::ShaderModule(vertSize, vertCode);
        spv_reflect::ShaderModule fragReflectMod = spv_reflect::ShaderModule(fragSize, fragCode);

        { // Create vertex shader input info
            uint32_t inputCount;
            vertReflectMod.EnumerateInputVariables(&inputCount, 0);
            SpvReflectInterfaceVariable** inputVariables = (SpvReflectInterfaceVariable**)alloca(inputCount*sizeof(SpvReflectInterfaceVariable*));
            vertReflectMod.EnumerateInputVariables(&inputCount, inputVariables);
            
            size_t stride = 0;
            for(int i = 0; i < inputCount; i++) stride += getFormatSize(static_cast<VkFormat>(inputVariables[i]->format));
            int binding = pipeline->addBinding(stride, VK_VERTEX_INPUT_RATE_VERTEX);

            size_t offset = 0;
            for(int i = 0; i < inputCount; i++) {
                VkFormat format = static_cast<VkFormat>(inputVariables[i]->format);
                pipeline->addAttribute(binding, inputVariables[i]->location, offset, format);
                offset += getFormatSize(format);
            }
        }

        GuiShader shaderResource = {};

        struct DescriptorInfo {
            uint set;
            uint binding;
            size_t size;
            GuiShader::DescriptorInfo* info;

            DescriptorInfo() {
                set = 0;
                binding = 0;
                size = 0;
                info = 0;
            }

            DescriptorInfo(uint set, uint binding, size_t size, GuiShader::DescriptorInfo* info) {
                this->set = set;
                this->binding = binding;
                this->size = size;
                this->info = info;
            }
        };

        std::list<DescriptorInfo> buffersToCreate;

        size_t pushOffset = 0;
        { // Create vertex shader descriptors and push constants
            uint32_t descBindCount;
            vertReflectMod.EnumerateDescriptorBindings(&descBindCount, 0); // TODO: Create buffers for uniform variables.
            SpvReflectDescriptorBinding** descriptorBindings = (SpvReflectDescriptorBinding**)alloca(descBindCount*sizeof(SpvReflectDescriptorBinding*));
            vertReflectMod.EnumerateDescriptorBindings(&descBindCount, descriptorBindings);

            for(int i = 0; i < descBindCount; i++) {
                pipeline->addDescriptors(descriptorBindings[i]->set, descriptorBindings[i]->binding, static_cast<VkDescriptorType>(descriptorBindings[i]->descriptor_type), descriptorBindings[i]->count, VK_SHADER_STAGE_VERTEX_BIT);

                GuiShader::DescriptorInfo info = { // TODO: Add to fragment
                    .set = descriptorBindings[i]->set,
                    .binding = descriptorBindings[i]->binding,
                    .type = static_cast<VkDescriptorType>(descriptorBindings[i]->descriptor_type)
                };
                auto pair = shaderResource.descriptors.insert({ descriptorBindings[i]->name, info });

                if(descriptorBindings[i]->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
                    buffersToCreate.push_back({
                        descriptorBindings[i]->set,
                        descriptorBindings[i]->binding,
                        descriptorBindings[i]->block.padded_size,
                        &pair.first->second
                    });
                }
            }

            uint32_t pushCount;
            vertReflectMod.EnumeratePushConstantBlocks(&pushCount, 0);
            SpvReflectBlockVariable** pushConstants = (SpvReflectBlockVariable**)alloca(pushCount*sizeof(SpvReflectBlockVariable*));
            vertReflectMod.EnumeratePushConstantBlocks(&pushCount, pushConstants);

            for(int i = 0; i < pushCount; i++) {
                pipeline->addPushConstant(pushOffset, pushConstants[i]->size, VK_SHADER_STAGE_VERTEX_BIT);
                pushOffset += pushConstants[i]->size;
            }
        }

        { // Create fragment shader descriptors and push constants
            uint32_t descBindCount;
            fragReflectMod.EnumerateDescriptorBindings(&descBindCount, 0); // TODO: Create buffers for uniform variables.
            SpvReflectDescriptorBinding** descriptorBindings = (SpvReflectDescriptorBinding**)alloca(descBindCount*sizeof(SpvReflectDescriptorBinding*));
            fragReflectMod.EnumerateDescriptorBindings(&descBindCount, descriptorBindings);

            for(int i = 0; i < descBindCount; i++) {
                pipeline->addDescriptors(descriptorBindings[i]->set, descriptorBindings[i]->binding, static_cast<VkDescriptorType>(descriptorBindings[i]->descriptor_type), descriptorBindings[i]->count, VK_SHADER_STAGE_FRAGMENT_BIT);

                GuiShader::DescriptorInfo info = { // TODO: Add to fragment
                    .set = descriptorBindings[i]->set,
                    .binding = descriptorBindings[i]->binding,
                    .type = static_cast<VkDescriptorType>(descriptorBindings[i]->descriptor_type),
                    .boundBuffer = 0
                };
                auto pair = shaderResource.descriptors.insert({ descriptorBindings[i]->name, info });

                if(descriptorBindings[i]->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
                    buffersToCreate.push_back({
                        descriptorBindings[i]->set,
                        descriptorBindings[i]->binding,
                        descriptorBindings[i]->block.padded_size,
                        &pair.first->second
                    });
                }
            }

            uint32_t pushCount;
            fragReflectMod.EnumeratePushConstantBlocks(&pushCount, 0);
            SpvReflectBlockVariable** pushConstants = (SpvReflectBlockVariable**)alloca(pushCount*sizeof(SpvReflectBlockVariable*));
            fragReflectMod.EnumeratePushConstantBlocks(&pushCount, pushConstants);

            for(int i = 0; i < pushCount; i++) {
                pipeline->addPushConstant(pushOffset, pushConstants[i]->size, VK_SHADER_STAGE_FRAGMENT_BIT);
                pushOffset += pushConstants[i]->size;
            }
        }

        if(pushOffset > 128) spdlog::warn("Combined push contant size exceeds the 128 byte limit.");

        // TODO: [11.09.2021] Change the size of this pipeline according to the current window size.
        if(!pipeline->assemble({ 1024, 720 })) {
            delete pipeline;
            return (gui_resource_handle)0;
        }
        shaderResource.pipeline = pipeline;

        for(auto& desc : buffersToCreate) {
            Buffer* buffer = new Buffer(desc.size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, VMA_MEMORY_USAGE_CPU_TO_GPU);
            GuiBuffer bufRes = { buffer };
            Resource* res = new Resource(bufRes, [](Resource& value) { 
                auto opt = value.get<GuiBuffer>();
                if(opt.has_value()) delete opt.value()->buffer;
            });
            gui_resource_handle handle = nextHandle;
            nextHandle++;
            resources.insert({ handle, res });
            
            desc.info->isDefault = true;
            desc.info->boundBuffer = handle;
            desc.info->boundOffset = 0;
            desc.info->boundLength = desc.size;

            pipeline->bindBufferToDescriptor(desc.set, desc.binding, buffer->getHandle(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, desc.size);
        }

        Resource* resource = new Resource(shaderResource, [](Resource& value) { 
            auto opt = value.get<GuiShader>();
            if(opt.has_value()) delete opt.value()->pipeline;
        });
        gui_resource_handle handle = nextHandle;
        resources.insert({ handle, resource });
        nextHandle++;

        return handle;
    };

    functions.bind_buffer_to_descriptor = [&](gui_resource_handle shader, const std::string& descName, gui_resource_handle buffer, size_t offset, size_t length) {
        auto shaderRef = getResource<GuiShader>(shader);
        if(!shaderRef.has_value()) return;
        auto desc = shaderRef.value()->descriptors.find(descName);
        if(desc == shaderRef.value()->descriptors.end()) return;

        auto bufferRef = getResource<GuiBuffer>(buffer);
        if(!bufferRef.has_value()) return;

        shaderRef.value()->pipeline->bindBufferToDescriptor(desc->second.set, desc->second.binding, bufferRef.value()->buffer->getHandle(), desc->second.type, offset, length);
        if(desc->second.boundBuffer != 0 && desc->second.isDefault) functions.destroy_resource(desc->second.boundBuffer);
        desc->second.boundBuffer = buffer;
        desc->second.boundOffset = offset;
        desc->second.boundLength = length;
        desc->second.isDefault = false;
    };

    functions.set_shader_variable = [&](gui_resource_handle shader, const std::string& descName, void* data, size_t offset, size_t length) {
        auto shaderRef = getResource<GuiShader>(shader);
        if(!shaderRef.has_value()) return;

        auto desc = shaderRef.value()->descriptors.find(descName);
        if(desc == shaderRef.value()->descriptors.end()) return;
        if(desc->second.boundBuffer == 0) return;

        auto buffer = getResource<GuiBuffer>(desc->second.boundBuffer);
        if(buffer.has_value()) {
            void* mapping = buffer.value()->buffer->map();
            memcpy((uint8_t*)mapping+offset, data, length);
            buffer.value()->buffer->unmap();
        }
    };

    functions.set_render_object_shader = [&](gui_resource_handle renderObject, gui_resource_handle shader) {
        auto shaderRef = getResource<GuiShader>(shader);
        if(!shaderRef.has_value()) return;

        auto renObjRef = getResource<RenderObject>(renderObject);
        if(!renObjRef.has_value()) return;

        renObjRef.value()->pipeline = shaderRef.value()->pipeline;
    };

    functions.set_render_layer = [&](gui_resource_handle renderObject, int layer) {
        auto renObjRef = getResource<RenderObject>(renderObject);
        if(renObjRef.has_value()) renObjRef.value()->layer = layer;
    };

    functions.create_mesh = [&]() {
        Mesh mesh = { };
        Resource* resource = new Resource(mesh, [](Resource& value) {
            auto opt = value.get<Mesh>();
            if(opt.has_value()) {
                if(opt.value()->indexBuffer != 0) delete opt.value()->indexBuffer;
                if(opt.value()->vertexBuffer != 0) delete opt.value()->vertexBuffer;
            }
        });

        gui_resource_handle handle = nextHandle;
        nextHandle++;
        resources.insert({ handle, resource });
        return handle;
    };

    functions.add_mesh_vertex_data = [&](gui_resource_handle mesh, const std::vector<uint8_t>& data, uint vertexCount) {
        auto meshRef = getResource<Mesh>(mesh);
        if(!meshRef.has_value()) return;

        Mesh* msh = meshRef.value();
        if(msh->indexBuffer == 0) msh->vertexCount = vertexCount;
        if(msh->vertexBuffer != 0) delete msh->vertexBuffer;
        msh->vertexBuffer = new Buffer(data.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_SHARING_MODE_EXCLUSIVE, VMA_MEMORY_USAGE_GPU_ONLY);

        Buffer srcBuf = Buffer(data.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE, VMA_MEMORY_USAGE_CPU_ONLY);
        srcBuf.store(data.data(), 0, data.size());

        Engine::getInstance()->transfer(srcBuf.getHandle(), msh->vertexBuffer->getHandle(), data.size());
    };

    functions.add_mesh_indices = [&](gui_resource_handle mesh, const std::vector<uint>& indices, uint vertexCount) {
        auto meshRef = getResource<Mesh>(mesh);
        if(!meshRef.has_value()) return;

        Mesh* msh = meshRef.value();
        msh->vertexCount = vertexCount;
        if(msh->indexBuffer != 0) delete msh->indexBuffer;
        msh->indexBuffer = new Buffer(indices.size()*sizeof(uint), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_SHARING_MODE_EXCLUSIVE, VMA_MEMORY_USAGE_GPU_ONLY);

        Buffer srcBuf = Buffer(indices.size()*sizeof(uint), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE, VMA_MEMORY_USAGE_CPU_ONLY);
        srcBuf.store(indices.data(), 0, indices.size());

        Engine::getInstance()->transfer(srcBuf.getHandle(), msh->indexBuffer->getHandle(), indices.size()*sizeof(uint));
    };

    functions.attach_mesh = [&](gui_resource_handle renderObject, gui_resource_handle mesh) {
        auto meshRef = getResource<Mesh>(mesh);
        if(!meshRef.has_value()) return;

        auto renObjRef = getResource<RenderObject>(renderObject);
        if(!renObjRef.has_value()) return;

        renObjRef.value()->mesh = meshRef.value();
    };

    instance = this;
}

GuiRenderer::~GuiRenderer() {
    instance = 0;
    for(auto& [handle, resource] : resources) delete resource;
}

void GuiRenderer::render(VkCommandBuffer cmd) {
    for(auto& callback : renderCallbacks) callback(1.667);

    GraphicsPipeline* currentPipeline = 0;
    for(auto& object : objectsToRender) {
        if(object->pipeline != currentPipeline) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object->pipeline->getHandle());
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object->pipeline->getLayout(), 0, 1, object->pipeline->getDescriptorSet(), 0, 0);
        }

        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmd, 0, 1, &object->mesh->vertexBuffer->getHandle(), offsets);

        if(object->mesh->indexBuffer != 0) {
            // Indexed draw
            vkCmdBindIndexBuffer(cmd, object->mesh->indexBuffer->getHandle(), 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(cmd, object->mesh->vertexCount, 1, 0, 0, 0);
        } else {
            // Regular draw
            vkCmdDraw(cmd, object->mesh->vertexCount, 1, 0, 0);
        }
    }
    objectsToRender.clear();
}

GuiRenderer* GuiRenderer::getInstance() {
    return instance;
}

const RenderEngine& GuiRenderer::getRenderEngineFunctions() {
    return functions;
}

void GuiRenderer::addRenderCallback(std::function<void(double)> callback) {
    renderCallbacks.push_back(callback);
}