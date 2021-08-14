#include <renderer/buffer_renderer.hpp>

using namespace unibox;

BufferRenderer::BufferRenderer(const std::string& material, size_t vertexCount, Buffer* buffer)
    : Renderer(material) {
    this->buffer = buffer;
    this->vertexCount = vertexCount;
}

BufferRenderer::~BufferRenderer() {

}

void BufferRenderer::setVertexCount(size_t vertexCount) {
    this->vertexCount = vertexCount;
}

void BufferRenderer::render(VkCommandBuffer buffer) {
    VkDeviceSize offsets[] = {0};
    VkBuffer buffers[] = {this->buffer->getHandle()};
    vkCmdBindVertexBuffers(buffer, 0, 1, buffers, offsets);

    vkCmdDraw(buffer, vertexCount, 1, 0, 0);
}