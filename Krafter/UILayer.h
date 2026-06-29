#pragma once

#include "Krafter/Hotbar.h"
#include "Krafter/Core/Layer.h"
#include "Krafter/Renderer/Font.h"
#include "Krafter/Renderer/Texture.h"
#include "Krafter/Renderer/UIRenderer.h"

namespace Krafter {

class Window;

class UILayer : public Layer {
public:
    // The UI renderer and sprite sheet (ui.png) are owned by the application and
    // shared across the UI layers; the HUD only borrows them.
    UILayer(Window& window, UIRenderer& renderer, Texture2D& uiTexture, Font& font, Hotbar& hotbar);

    // Shows or hides the whole HUD. Hidden while the inventory screen is up, which
    // draws its own hotbar row, so the bottom hotbar doesn't show through.
    void SetVisible(bool visible)
    {
        m_Visible = visible;
    }

private:
    void OnRender() override;
    void OnEvent(Event& event) override;

    void DrawCrosshair();
    void DrawHotbar();

    Window& m_Window;
    Hotbar& m_Hotbar;
    UIRenderer& m_Renderer;
    Texture2D& m_UITexture;
    Font& m_Font;

    // The block and item atlases are the HUD's own, used only for hotbar icons.
    Texture2D m_BlockTexture;
    Texture2D m_ItemTexture;

    bool m_Visible = true;
};

} // namespace Krafter
