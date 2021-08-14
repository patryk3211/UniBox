#include <renderer/renderer.hpp>

#include <algorithm>

using namespace unibox;

std::unordered_map<std::string, GraphicsPipeline*> Renderer::materials = std::unordered_map<std::string, GraphicsPipeline*>();
std::unordered_map<GraphicsPipeline*, std::list<Renderer*>> Renderer::renderers = std::unordered_map<GraphicsPipeline*, std::list<Renderer*>>();

Renderer::Renderer(const std::string& material) {
    auto node = materials.find(material);
    if(node == materials.end()) return;
    this->material = node->second;

    auto renNode = renderers.find(this->material);
    if(renNode == renderers.end()) return;
    auto& rens = renNode->second;
    rens.push_back(this);
}

Renderer::~Renderer() {
    renderers[material].remove(this);
}

void Renderer::render(VkCommandBuffer buffer) {
    
}

void Renderer::registerMaterial(const std::string& name, GraphicsPipeline* pipeline) {
    materials.insert({ name, pipeline });
    renderers.insert({ pipeline, std::list<Renderer*>() });
}

void Renderer::renderAll(VkCommandBuffer buffer) {
    for(auto node = renderers.begin(); node != renderers.end(); node++) {
        if(node->second.size() == 0) continue;
        vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, node->first->getHandle());

        vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, node->first->getLayout(), 0, 1, node->first->getDescriptorSet(), 0, 0);
        for(auto renderer = node->second.begin(); renderer != node->second.end(); renderer++) {
            Renderer* ren = *renderer;
            ren->render(buffer);
        }
    }
}