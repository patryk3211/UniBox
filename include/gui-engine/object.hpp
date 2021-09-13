#pragma once

#include <gui-engine/types.hpp>

#include <glm/mat4x4.hpp>

namespace unibox::gui {
    class GuiEngine;
    struct RenderEngine;

    class GuiObject {
        int layer;

        gui_handle handle;

        gui_resource_handle renderObjectHandle;

        gui_resource_handle shader;
        gui_resource_handle mesh;

        double posX;
        double posY;

        double scaleX;
        double scaleY;

        glm::mat4 transformMatrix;

        void rebuildMatrix();
    protected:
        GuiEngine& guiEngine;
        const RenderEngine& renderEngine;
    public:
        GuiObject(GuiEngine& engine, int layer, double posX, double posY, double scaleX, double scaleY);
        ~GuiObject();

        int getLayer();
        void setLayer(int layer);

        gui_resource_handle getShader();
        gui_resource_handle getMesh();

        void setShader(gui_resource_handle shader);
        void setMesh(gui_resource_handle mesh);

        double getX();
        double getY();
        double getWidth();
        double getHeight();

        void setX(double value);
        void setY(double value);
        void setWidth(double value);
        void setHeight(double value);

        virtual void render(double frameTime);

        virtual void mouseHover(double x, double y);
        virtual void mouseClick(double x, double y, int button);

        bool operator==(const GuiObject& other);
    };
}
