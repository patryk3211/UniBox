#include <util/finalizer.hpp>

using namespace unibox;

Finalizer* Finalizer::instance = 0;

Finalizer::Finalizer() { 
    instance = this;
}

Finalizer::~Finalizer() {
    std::for_each(callbacks.begin(), callbacks.end(), [](std::function<void(void)> callback) {
        callback();
    });
}

void Finalizer::addCallback(std::function<void(void)> callback) {
    instance->callbacks.push_front(callback);
}