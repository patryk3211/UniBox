#pragma once

#include <CL/cl.hpp>

#include <vector>

namespace unibox {
    class ClEngine {
        static ClEngine* instance;

        cl::Device device;
        cl::Context context;

        bool valid;
    public:
        ClEngine();
        ~ClEngine();

        cl::Context& getContext();
        cl::Device& getDevice();

        static ClEngine* getInstance();
    };
}