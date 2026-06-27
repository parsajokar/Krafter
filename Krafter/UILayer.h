#pragma once

#include "Krafter/Hotbar.h"
#include "Krafter/Core/Layer.h"
#include "Krafter/Renderer/Texture.h"
#include "Krafter/Renderer/UIRenderer.h"
#include "Krafter/World/Block.h"

namespace Krafter {

class Window;

class UILayer : public Layer {
public:
    // The UI renderer and sprite sheet (ui.png) are owned by the application and
    // shared across the UI layers; the HUD only borrows them.
    UILayer(Window& window, UIRenderer& renderer, Texture2D& uiTexture, Hotbar& hotbar);

private:
    void OnRender() override;
    void OnEvent(Event& event) override;

    void DrawCrosshair();
    void DrawHotbar();

    // Draws a block's side texture from the world atlas, used for hotbar icons.
    void DrawBlockIcon(Block block, const glm::vec2& position, const glm::vec2& size);

    Window& m_Window;
    Hotbar& m_Hotbar;
    UIRenderer& m_Renderer;
    Texture2D& m_UITexture;

    // The block atlas is the HUD's own, used only for the hotbar icons.
    Texture2D m_BlockTexture;
};

} // namespace Krafter
