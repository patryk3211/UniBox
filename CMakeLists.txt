cmake_minimum_required(VERSION 3.9)

project(UniBox VERSION 0.0.1 LANGUAGES C CXX)

set(SOURCES src/main.cpp
            src/vk-engine/window.cpp
            src/vk-engine/engine.cpp
            src/vk-engine/commandpool.cpp
            src/vk-engine/commandbuffer.cpp
            src/vk-engine/gfxpipeline.cpp
            src/vk-engine/computepipeline.cpp
            src/vk-engine/shader.cpp
            src/vk-engine/vma.cpp
            src/vk-engine/buffer.cpp
            src/renderer/renderer.cpp
            src/renderer/buffer_renderer.cpp
            src/renderer/camera.cpp
            src/compute/meshgen.cpp
            src/compute/simulator.cpp)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

add_executable(unibox ${SOURCES})
set_property(TARGET unibox PROPERTY CXX_STANDARD 17)
set_property(TARGET unibox PROPERTY C_STANDARD 11)

target_include_directories(unibox PRIVATE ${CMAKE_SOURCE_DIR}/include
                                  PRIVATE ${CMAKE_SOURCE_DIR}/spdlog/include
                                  PRIVATE ${CMAKE_SOURCE_DIR}/vk-bootstrap/src
                                  PRIVATE ${SMAKE_SOURCE_DIR}/glm)

target_link_libraries(unibox "-lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi" spdlog::spdlog vk-bootstrap::vk-bootstrap)

add_custom_target(unibox-prepare-run
                  mkdir -p ${CMAKE_SOURCE_DIR}/run && cp ${CMAKE_BINARY_DIR}/unibox ${CMAKE_SOURCE_DIR}/run/unibox
                  DEPENDS unibox
                  BYPRODUCTS ${CMAKE_SOURCE_DIR}/run/unibox)

add_custom_target(unibox-run
                  ${CMAKE_SOURCE_DIR}/run/unibox
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/run
                  DEPENDS unibox-prepare-run)

add_subdirectory(spdlog)
add_subdirectory(vk-bootstrap)
