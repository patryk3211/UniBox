#pragma once

#include <list>
#include <algorithm>

namespace unibox {
    class Finalizer {
        static Finalizer* instance;

        std::list<std::function<void(void)>> callbacks;
    public:
        Finalizer();
        ~Finalizer();

        static void addCallback(std::function<void(void)> callback);
    };
}