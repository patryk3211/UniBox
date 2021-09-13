#include <gui-engine/object.hpp>

namespace unibox::gui {
    class Image : public GuiObject {
        gui_resource_handle tex;

    public:
        Image(GuiEngine& engine, gui_resource_handle shader, gui_resource_handle tex, double posX, double posY, double width, double height);
        ~Image();

        virtual void render(double frameTime, double x, double y);
    };
}