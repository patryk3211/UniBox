#pragma once

#include <gui-engine/object.hpp>
#include <glm/vec2.hpp>
#include <util/text.hpp>

#include <string>
#include <optional>

namespace unibox {
    class Tooltip : public gui::GuiObject {
        static gui::gui_resource_handle textureHandle;
        std::string text;

        std::optional<glm::vec2> position;
        gui::gui_resource_handle meshHandle;

        gui::gui_resource_handle textObjRO;
        gui::gui_resource_handle fontTex;

        util::Text textObj;
        float textSize;

        bool visible;

        static void ShaderInitFunc(gui::GuiEngine& engine, gui::gui_resource_handle handle);

        void init(const std::string& font_name);
    public:
        Tooltip(const std::string& font_name, gui::GuiEngine& engine, float textSize, const std::string& text);
        Tooltip(const std::string& font_name, gui::GuiEngine& engine, float textSize, float x, float y, const std::string& text);
        ~Tooltip();

        void setVisible(bool value);
        void setText(const std::string& text);

        virtual void render(double frameTime, double x, double y);
    };
}