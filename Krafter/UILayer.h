#pragma once

#include "Krafter/Layer.h"
#include "Krafter/Renderer/Texture.h"
#include "Krafter/Renderer/UIRenderer.h"

namespace Krafter {

class Window;

class UILayer : public Layer {
public:
    UILayer(Window& window);

private:
    void OnRender() override;

    void DrawCrosshair();
    void DrawHotbar();

    void DrawSprite(
        const glm::vec2& spritePos, const glm::vec2& spriteSize,
        const glm::vec2& position, const glm::vec2& size, float opacity = 1.0f);

    Window& m_Window;
    UIRenderer m_Renderer;
    Texture2D m_UITexture;
};

} // namespace Krafter
