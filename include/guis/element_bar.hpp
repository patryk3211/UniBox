#pragma once

#include <gui-engine/object.hpp>
#include <simulator/particle.hpp>
#include <glm/mat4x4.hpp>

#include <util/text.hpp>

namespace unibox {
    class ElementBar : public gui::GuiObject {
        struct HotbarSlot {
            gui::GuiEngine& guiEngine;
            const gui::RenderEngine& renderEngine;

            glm::mat4 transform;
            bool isSelected;
            gui::gui_resource_handle background_RO;
            gui::gui_resource_handle particle_icon_RO;

            gui::gui_resource_handle background_TEX;
            gui::gui_resource_handle icon_TEX;

            Particle* particle;

            HotbarSlot(uint x, uint y, gui::GuiEngine& engine, gui::gui_resource_handle backgroundTexture, gui::gui_resource_handle iconTextureAtlas);
            ~HotbarSlot();

            void render();
        };

        gui::gui_resource_handle fontAtlas;
        gui::gui_resource_handle textRO;

        gui::gui_resource_handle iconAtlas;
        gui::gui_resource_handle slotTexture;

        HotbarSlot* hotbar[10];
    public:
        ElementBar(gui::GuiEngine& engine);
        ~ElementBar();

        virtual void render(double frameTime, double x, double y);
    };
}