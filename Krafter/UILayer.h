#pragma once

#include "Krafter/Layer.h"
#include "Krafter/Renderer/UIRenderer.h"

namespace Krafter {

class Window;

class UILayer : public Layer {
public:
    UILayer(Window& window);

private:
    void OnRender() override;

    void DrawCrosshair();

    Window& m_Window;
    UIRenderer m_Renderer;
};

} // namespace Krafter
