cmake_minimum_required(VERSION 3.10)

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
            src/vk-engine/image.cpp
            src/renderer/renderer.cpp
            src/renderer/buffer_renderer.cpp
            src/renderer/camera.cpp
            src/renderer/gui_renderer.cpp
            src/compute/meshgen.cpp
            src/compute/simulator.cpp
            src/simulator/particle.cpp
            src/simulator/particle_grid.cpp
            src/util/shader_assembler.cpp
            src/util/savefile.cpp
            src/util/finalizer.cpp
            src/util/format.cpp
            src/util/texture_atlas.cpp
            src/util/base64.cpp
            src/util/font.cpp
            src/util/text.cpp
            src/util/global_resources.cpp
            src/cl-engine/engine.cpp
            src/gui-engine/engine.cpp
            src/gui-engine/object.cpp
            src/gui-engine/image.cpp
            src/gui-engine/button.cpp
            src/guis/element_bar.cpp
            src/guis/tooltip.cpp
            src/spirv_reflect.c
            src/stb/stb_image.c)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

add_executable(unibox ${SOURCES})
set_property(TARGET unibox PROPERTY CXX_STANDARD 17)
set_property(TARGET unibox PROPERTY C_STANDARD 11)

target_include_directories(unibox PRIVATE ${CMAKE_SOURCE_DIR}/include
                                  PRIVATE ${CMAKE_SOURCE_DIR}/spdlog/include
                                  PRIVATE ${CMAKE_SOURCE_DIR}/vk-bootstrap/src
                                  PRIVATE ${CMAKE_SOURCE_DIR}/glm
                                  PRIVATE ${CMAKE_SOURCE_DIR}/sol2/include
                                  PRIVATE ${CMAKE_SOURCE_DIR}/Lua/lua-5.4.3/include
                                  PRIVATE ${CMAKE_SOURCE_DIR}/json/include)

target_link_libraries(unibox "-lglfw -lvulkan -lOpenCL -ldl -lX11 -lXxf86vm -lXrandr -lXi -lglslang -lMachineIndependent -lOSDependent -lGenericCodeGen -lOGLCompiler -lSPIRV -lpthread -lfreetype" spdlog::spdlog vk-bootstrap::vk-bootstrap sol2::sol2 nlohmann_json::nlohmann_json)

add_custom_target(unibox-prepare-run
                  mkdir -p ${CMAKE_SOURCE_DIR}/run && cp ${CMAKE_BINARY_DIR}/unibox ${CMAKE_SOURCE_DIR}/run/unibox
                  DEPENDS unibox particles run/resources
                  BYPRODUCTS ${CMAKE_SOURCE_DIR}/run/unibox)

add_custom_target(unibox-run
                  ${CMAKE_SOURCE_DIR}/run/unibox
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/run
                  DEPENDS unibox-prepare-run)

add_custom_target(particles
                  cp -r ${CMAKE_SOURCE_DIR}/particles ${CMAKE_SOURCE_DIR}/run
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_custom_target(resources
                  cp -r ${CMAKE_SOURCE_DIR}/resources ${CMAKE_SOURCE_DIR}/run
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_subdirectory(spdlog)
add_subdirectory(vk-bootstrap)
add_subdirectory(sol2)
add_subdirectory(Lua)
add_subdirectory(json)
