#include <renderer/gui_renderer.hpp>

#include <spirv/spirv_reflect.h>
#include <util/format.hpp>

#include <spdlog/spdlog.h>

#include <future>

using namespace unibox;
using namespace unibox::gui;

GuiRenderer* GuiRenderer::instance = 0;

GuiRenderer::GuiRenderer(uint width, uint height, Window& window) : window(window) {
    nextHandle = 1;

    functions.width = width;
    functions.height = height;

    // Create Render Object
    functions.create_render_object = [&](gui_resource_handle shader) {
        auto shaderRef = getResource<GuiShader>(shader);
        if(!shaderRef.has_value()) return (gui_resource_handle)0;
        RenderObject object = {};

        object.shader = shaderRef.value();
        object.objectSet = object.shader->pipeline->allocateSet(0);
        for(auto& desc : object.shader->set0BufferCreate) {
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

            object.shader->pipeline->bindBufferToDescriptor(object.objectSet, desc.binding, buffer->getHandle(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, desc.size);
        }

        Resource* resource = new Resource(object);
        gui_resource_handle handle = nextHandle;
        resources.insert({ handle, resource });
        nextHandle++;

        return handle;
    };

    // Destroy Resource
    functions.destroy_resource = [&](gui_resource_handle handle) { 
        if(handle != 0 && resources.find(handle) != resources.end()) {
            delete resources[handle];
            resources.erase(handle);
        }
    };

    // Render Object
    functions.render_object = [&](gui_resource_handle handle) {
        std::optional<RenderObject*> obj = getResource<RenderObject>(handle);
        if(obj.has_value()) {
            if(obj.value()->shader == 0) return;
            RenderObject* renderObject = obj.value();
            renderActions.push([renderObject](RenderingState state, VkCommandBuffer cmd) {
                if(state.currentShader != renderObject->shader) {
                    state.currentShader = renderObject->shader;
                    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, renderObject->shader->pipeline->getHandle());
                    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, renderObject->shader->pipeline->getLayout(), 1, 1, &renderObject->shader->shaderSet, 0, 0);
                }

                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, renderObject->shader->pipeline->getLayout(), 0, 1, &renderObject->objectSet, 0, 0);

                VkDeviceSize offsets[] = {0};
                vkCmdBindVertexBuffers(cmd, 0, 1, &renderObject->mesh->vertexBuffer->getHandle(), offsets);

                if(renderObject->shader->pushSizeVertex > 0) vkCmdPushConstants(cmd, state.currentShader->pipeline->getLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, renderObject->shader->pushSizeVertex, renderObject->shader->pushConstant);
                if(renderObject->shader->pushSizeFragment > 0) vkCmdPushConstants(cmd, state.currentShader->pipeline->getLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, renderObject->shader->pushSizeVertex, renderObject->shader->pushSizeFragment, renderObject->shader->pushConstant+renderObject->shader->pushSizeVertex);

                if(renderObject->mesh->indexBuffer != 0) {
                    // Indexed draw
                    vkCmdBindIndexBuffer(cmd, renderObject->mesh->indexBuffer->getHandle(), 0, VK_INDEX_TYPE_UINT32);
                    vkCmdDrawIndexed(cmd, renderObject->mesh->vertexCount, 1, 0, 0, 0);
                } else {
                    // Regular draw
                    vkCmdDraw(cmd, renderObject->mesh->vertexCount, 1, 0, 0);
                }
            });
        }
    };

    // Create Shader
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
            vertReflectMod.EnumerateDescriptorBindings(&descBindCount, 0);
            SpvReflectDescriptorBinding** descriptorBindings = (SpvReflectDescriptorBinding**)alloca(descBindCount*sizeof(SpvReflectDescriptorBinding*));
            vertReflectMod.EnumerateDescriptorBindings(&descBindCount, descriptorBindings);

            for(int i = 0; i < descBindCount; i++) {
                pipeline->addDescriptors(descriptorBindings[i]->set, descriptorBindings[i]->binding, static_cast<VkDescriptorType>(descriptorBindings[i]->descriptor_type), descriptorBindings[i]->count, VK_SHADER_STAGE_VERTEX_BIT);

                GuiShader::DescriptorInfo info = {
                    .set = descriptorBindings[i]->set,
                    .binding = descriptorBindings[i]->binding,
                    .size = descriptorBindings[i]->block.padded_size,
                    .type = static_cast<VkDescriptorType>(descriptorBindings[i]->descriptor_type)
                };
                GuiShader::DescriptorInfo* infoPtr = new GuiShader::DescriptorInfo(info);
                auto pair = shaderResource.descriptors.insert({ descriptorBindings[i]->name, infoPtr });
                for(int j = 0; j < descriptorBindings[i]->block.member_count; j++) {
                    auto& member = descriptorBindings[i]->block.members[j];
                    GuiShader::DescriptorMemberInfo mem = {
                        infoPtr,
                        member.absolute_offset
                    };
                    shaderResource.descriptorMembers.insert({ member.name, mem });
                }

                if(descriptorBindings[i]->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
                    if(descriptorBindings[i]->set == 1) {
                        buffersToCreate.push_back({
                            descriptorBindings[i]->set,
                            descriptorBindings[i]->binding,
                            descriptorBindings[i]->block.padded_size,
                            infoPtr
                        });
                    } else if(descriptorBindings[i]->set == 0) {
                        GuiShader::BufferCreateInfo descInf = {};
                        descInf.size = descriptorBindings[i]->block.padded_size;
                        descInf.binding = descriptorBindings[i]->binding;
                        descInf.info = infoPtr;
                        shaderResource.set0BufferCreate.push_back(descInf);
                    }
                }
            }

            uint32_t pushCount;
            vertReflectMod.EnumeratePushConstantBlocks(&pushCount, 0);
            SpvReflectBlockVariable** pushConstants = (SpvReflectBlockVariable**)alloca(pushCount*sizeof(SpvReflectBlockVariable*));
            vertReflectMod.EnumeratePushConstantBlocks(&pushCount, pushConstants);

            for(int i = 0; i < pushCount; i++) {
                pipeline->addPushConstant(pushOffset, pushConstants[i]->size, VK_SHADER_STAGE_VERTEX_BIT);
                for(int j = 0; j < pushConstants[i]->member_count; j++) 
                    shaderResource.pushConstants.insert({ pushConstants[i]->members[j].name, pushOffset+pushConstants[i]->members[j].absolute_offset });
                pushOffset += pushConstants[i]->padded_size;
            }
        }
        shaderResource.pushSizeVertex = pushOffset;

        { // Create fragment shader descriptors and push constants
            uint32_t descBindCount;
            fragReflectMod.EnumerateDescriptorBindings(&descBindCount, 0);
            SpvReflectDescriptorBinding** descriptorBindings = (SpvReflectDescriptorBinding**)alloca(descBindCount*sizeof(SpvReflectDescriptorBinding*));
            fragReflectMod.EnumerateDescriptorBindings(&descBindCount, descriptorBindings);

            for(int i = 0; i < descBindCount; i++) {
                pipeline->addDescriptors(descriptorBindings[i]->set, descriptorBindings[i]->binding, static_cast<VkDescriptorType>(descriptorBindings[i]->descriptor_type), descriptorBindings[i]->count, VK_SHADER_STAGE_FRAGMENT_BIT);

                GuiShader::DescriptorInfo info = {
                    .set = descriptorBindings[i]->set,
                    .binding = descriptorBindings[i]->binding,
                    .size = descriptorBindings[i]->block.padded_size,
                    .type = static_cast<VkDescriptorType>(descriptorBindings[i]->descriptor_type)
                };
                GuiShader::DescriptorInfo* infoPtr = new GuiShader::DescriptorInfo(info);
                auto pair = shaderResource.descriptors.insert({ descriptorBindings[i]->name, infoPtr });
                for(int j = 0; j < descriptorBindings[i]->block.member_count; j++) {
                    auto& member = descriptorBindings[i]->block.members[j];
                    GuiShader::DescriptorMemberInfo mem = {
                        infoPtr,
                        member.absolute_offset
                    };
                    shaderResource.descriptorMembers.insert({ member.name, mem });
                }

                if(descriptorBindings[i]->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
                    if(descriptorBindings[i]->set == 1) {
                        buffersToCreate.push_back({
                            descriptorBindings[i]->set,
                            descriptorBindings[i]->binding,
                            descriptorBindings[i]->block.padded_size,
                            infoPtr
                        });
                    } else if(descriptorBindings[i]->set == 0) {
                        GuiShader::BufferCreateInfo descInf = {};
                        descInf.size = descriptorBindings[i]->block.padded_size;
                        descInf.binding = descriptorBindings[i]->binding;
                        descInf.info = infoPtr;
                        shaderResource.set0BufferCreate.push_back(descInf);
                    }
                }
            }

            uint32_t pushCount;
            fragReflectMod.EnumeratePushConstantBlocks(&pushCount, 0);
            SpvReflectBlockVariable** pushConstants = (SpvReflectBlockVariable**)alloca(pushCount*sizeof(SpvReflectBlockVariable*));
            fragReflectMod.EnumeratePushConstantBlocks(&pushCount, pushConstants);

            for(int i = 0; i < pushCount; i++) {
                pipeline->addPushConstant(pushOffset, pushConstants[i]->size, VK_SHADER_STAGE_FRAGMENT_BIT);
                for(int j = 0; j < pushConstants[i]->member_count; j++) 
                    shaderResource.pushConstants.insert({ pushConstants[i]->members[j].name, pushOffset+pushConstants[i]->members[j].absolute_offset });
                pushOffset += pushConstants[i]->padded_size;
            }
        }
        shaderResource.pushSizeFragment = pushOffset-shaderResource.pushSizeVertex;

        if(pushOffset > 128) spdlog::warn("Combined push contant size exceeds the 128 byte limit, things may break!");

        if(pushOffset > 0) {
            shaderResource.pushConstant = new uint8_t[pushOffset];
            memset(shaderResource.pushConstant, 0, pushOffset);
        }

        pipeline->setDescriptorAllocate(false);
        if(!pipeline->assemble({ functions.width, functions.height })) {
            delete pipeline;
            return (gui_resource_handle)0;
        }
        shaderResource.pipeline = pipeline;
        shaderResource.shaderSet = pipeline->allocateSet(1); // Bound once per shader.

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

            glm::mat4 m = glm::mat4(1);
            buffer->load(&m, 0, sizeof(m));

            pipeline->bindBufferToDescriptor(shaderResource.shaderSet, desc.binding, buffer->getHandle(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, desc.size);
        }

        Resource* resource = new Resource(shaderResource, [](Resource& value) { 
            auto opt = value.get<GuiShader>();
            if(opt.has_value()) {
                delete opt.value()->pipeline;
                for(auto& [name, desc] : opt.value()->descriptors) delete desc;
            }
        });
        gui_resource_handle handle = nextHandle;
        resources.insert({ handle, resource });
        nextHandle++;

        return handle;
    };

    // Bind Buffer to Shader Descriptor
    functions.bind_buffer_to_descriptor = [&](gui_resource_handle resource, const std::string& descName, gui_resource_handle buffer, size_t offset, size_t length) {
        auto renObjRef = getResource<RenderObject>(resource);
        GuiShader* shdr = 0;
        RenderObject* gfx = 0;
        if(!renObjRef.has_value()) {
            auto shaderRef = getResource<GuiShader>(resource);
            if(!shaderRef.has_value()) return;
            shdr = shaderRef.value();
        } else {
            shdr = renObjRef.value()->shader;
            gfx = renObjRef.value();
        }
        auto desc = shdr->descriptors.find(descName);
        if(desc == shdr->descriptors.end()) return;

        auto bufferRef = getResource<GuiBuffer>(buffer);
        if(!bufferRef.has_value()) return;

        Buffer* bfr = bufferRef.value()->buffer;
        renderActions.push([desc, gfx, this, buffer, offset, length, bfr, shdr](RenderingState state, VkCommandBuffer cmd) {
            if(desc->second->set == 1) shdr->pipeline->bindBufferToDescriptor(shdr->shaderSet, desc->second->binding, bfr->getHandle(), desc->second->type, offset, length);
            else if(desc->second->set == 0) shdr->pipeline->bindBufferToDescriptor(gfx->objectSet, desc->second->binding, bfr->getHandle(), desc->second->type, offset, length);
            if(desc->second->boundBuffer != 0 && desc->second->isDefault) functions.destroy_resource(desc->second->boundBuffer);
            desc->second->boundBuffer = buffer;
            desc->second->boundOffset = offset;
            desc->second->boundLength = length;
            desc->second->isDefault = false;
        });
    };

    // Set Shader Variable
    functions.set_shader_variable = [&](gui_resource_handle resource, const std::string& descName, const void* data, size_t offset, size_t length) {
        GuiShader* shdr;
        {
            auto renObjRef = getResource<RenderObject>(resource);
            if(!renObjRef.has_value()) {
                auto shaderRef = getResource<GuiShader>(resource);
                if(!shaderRef.has_value()) return;
                shdr = shaderRef.value();
            } else {
                shdr = renObjRef.value()->shader;
            }
        }
        
        GuiShader::DescriptorInfo* descInfo;

        auto desc = shdr->descriptorMembers.find(descName);
        if(desc == shdr->descriptorMembers.end()) {
            // Check push constants.
            auto cons = shdr->pushConstants.find(descName);
            if(cons != shdr->pushConstants.end()) {
                uint8_t* dataCpy = new uint8_t[length];
                memcpy(dataCpy, data, length);
                renderActions.push([dataCpy, cons, offset, length, shdr](RenderingState state, VkCommandBuffer cmd) {
                    memcpy(shdr->pushConstant+cons->second+offset, dataCpy, length);
                    delete[] dataCpy;
                });
                return;
            } else {
                // Check uniform block names.
                auto descBlck = shdr->descriptors.find(descName);
                if(descBlck == shdr->descriptors.end()) return;

                descInfo = descBlck->second;
            }
        } else {
            descInfo = desc->second.descriptor;
            offset += desc->second.offset;
        }
        if(descInfo->boundBuffer == 0) return;

        auto buffer = getResource<GuiBuffer>(descInfo->boundBuffer);
        if(buffer.has_value()) {
            Buffer* bfr = buffer.value()->buffer;
            uint8_t* dataCpy = new uint8_t[length];
            memcpy(dataCpy, data, length);
            renderActions.push([bfr, offset, dataCpy, length](RenderingState state, VkCommandBuffer cmd) {
                void* mapping = bfr->map();
                memcpy((uint8_t*)mapping+offset, dataCpy, length);
                bfr->unmap();
                delete[] dataCpy;
            });
        }
    };

    // Create Mesh
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

    // Add Mesh Vertex Data
    functions.add_mesh_vertex_data = [&](gui_resource_handle mesh, const std::vector<uint8_t>& data, uint vertexCount) {
        auto meshRef = getResource<Mesh>(mesh);
        if(!meshRef.has_value()) return;

        Mesh* msh = meshRef.value();
        if(msh->indexBuffer == 0) msh->vertexCount = vertexCount;
        if(msh->vertexBuffer != 0) { 
            Buffer* buf = msh->vertexBuffer;
            resourcesToDelete.push_back({ 5, [buf](){ delete buf; } }); // Wait 5 render cycles before deleting the buffer to make sure that it is not in use.
        }
        msh->vertexBuffer = new Buffer(data.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_SHARING_MODE_EXCLUSIVE, VMA_MEMORY_USAGE_GPU_ONLY);

        Buffer srcBuf = Buffer(data.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE, VMA_MEMORY_USAGE_CPU_ONLY);
        srcBuf.store(data.data(), 0, data.size());

        Engine::getInstance()->transfer(srcBuf.getHandle(), msh->vertexBuffer->getHandle(), data.size());
    };

    // Add Mesh Indicies
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

    // Attach Mesh to Render Object
    functions.attach_mesh = [&](gui_resource_handle renderObject, gui_resource_handle mesh) {
        auto meshRef = getResource<Mesh>(mesh);
        if(!meshRef.has_value()) return;

        auto renObjRef = getResource<RenderObject>(renderObject);
        if(!renObjRef.has_value()) return;

        renObjRef.value()->mesh = meshRef.value();
    };

    // Bind Texture to Shader Descriptor
    functions.bind_texture_to_descriptor = [&](gui_resource_handle resource, const std::string& descName, gui_resource_handle image) {
        auto renObjRef = getResource<RenderObject>(resource);
        GuiShader* shdr = 0;
        RenderObject* gfx = 0;
        if(!renObjRef.has_value()) {
            auto shaderRef = getResource<GuiShader>(resource);
            if(!shaderRef.has_value()) return;
            shdr = shaderRef.value();
        } else {
            shdr = renObjRef.value()->shader;
            gfx = renObjRef.value();
        }
        auto desc = shdr->descriptors.find(descName);
        if(desc == shdr->descriptors.end()) return;

        auto imageRef = getResource<Texture>(image);
        if(!imageRef.has_value()) return;

        Image* img = imageRef.value()->image;
        
        renderActions.push([desc, img, gfx, shdr](RenderingState state, VkCommandBuffer cmd) {
            if(desc->second->set == 1) shdr->pipeline->bindImageToDescriptor(shdr->shaderSet, desc->second->binding, img->getImageView(), img->getSampler(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, desc->second->type);
            else if(desc->second->set == 0 && gfx != 0) shdr->pipeline->bindImageToDescriptor(gfx->objectSet, desc->second->binding, img->getImageView(), img->getSampler(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, desc->second->type);
        });
    };

    // Create Texture
    functions.create_texture = [&](uint width, uint height, const void* data, ImageFormat format, ImageFilter minFilter, ImageFilter magFilter) {
        VkFormat formatVk;
        int formatSize = 0;
        switch(format) {
            case R8G8B8A8:
                formatVk = VK_FORMAT_R8G8B8A8_SRGB;
                formatSize = 4;
                break;
            case R8G8B8:
                formatVk = VK_FORMAT_R8G8B8_SRGB;
                formatSize = 3;
                break;
            case R8G8:
                formatVk = VK_FORMAT_R8G8_SRGB;
                formatSize = 2;
                break;
            case R8:
                formatVk = VK_FORMAT_R8_SRGB;
                formatSize = 1;
                break;
            default: return (gui_resource_handle)0;
        }
        Image* img = new Image(width, height, VK_IMAGE_USAGE_SAMPLED_BIT, VK_SHARING_MODE_EXCLUSIVE, formatVk, VK_IMAGE_TILING_OPTIMAL);
        img->loadImage(data, (width*height)*formatSize);
        VkFilter minFilt;
        VkFilter magFilt;
        switch(minFilter) {
            case LINEAR: minFilt = VK_FILTER_LINEAR; break;
            case NEAREST: minFilt = VK_FILTER_NEAREST; break;
            default: return (gui_resource_handle)0;
        }
        switch(magFilter) {
            case LINEAR: magFilt = VK_FILTER_LINEAR; break;
            case NEAREST: magFilt = VK_FILTER_NEAREST; break;
            default: return (gui_resource_handle)0;
        }
        img->createSampler(minFilt, magFilt, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
    
        Texture tex = { img };
        Resource* res = new Resource(tex, [](Resource& value) {
            auto opt = value.get<Texture>();
            if(opt.has_value()) {
                delete opt.value()->image;
            }
        });

        gui_resource_handle handle = nextHandle;
        nextHandle++;
        resources.insert({ handle, res });
        return handle;
    };

    instance = this;
}

GuiRenderer::~GuiRenderer() {
    instance = 0;
    for(auto& [handle, resource] : resources) delete resource;
    for(auto& resource : resourcesToDelete) resource.deleter();
}

void GuiRenderer::render(VkCommandBuffer cmd) {
    bool finished = false;
    std::async(std::launch::async, [&](){
        glm::vec2 cursor = window.getCursorPos();
        for(auto& callback : renderCallbacks) callback(1.667, cursor.x, cursor.y);
        finished = true;
        // While the main thread is rendering update the delete list
        auto it = resourcesToDelete.begin();
        while(it != resourcesToDelete.end()) {
            if(it->wait == 0) {
                it->deleter();
                resourcesToDelete.erase(it++);
            } else { 
                it->wait--;
                it++;
            }
        }
    });

    GuiShader* shader = 0;
    RenderingState state = {
        .currentShader = shader
    };

    do {
        while(!renderActions.empty()) {
            renderActions.front()(state, cmd);
            renderActions.pop();
        }
    } while(!finished);
}

GuiRenderer* GuiRenderer::getInstance() {
    return instance;
}

const RenderEngine& GuiRenderer::getRenderEngineFunctions() {
    return functions;
}

void GuiRenderer::addRenderCallback(std::function<void(double, double, double)> callback) {
    renderCallbacks.push_back(callback);
}
