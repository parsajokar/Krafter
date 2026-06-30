#pragma once

#include "Krafter/Hotbar.h"
#include "Krafter/Renderer/Texture.h"
#include "Krafter/UIScreen.h"

namespace Krafter {

class Window;
class UIRenderer;
class Font;

class UILayer : public UIScreen {
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

    Hotbar& m_Hotbar;

    // The block and item atlases are the HUD's own, used only for hotbar icons.
    Texture2D m_BlockTexture;
    Texture2D m_ItemTexture;

    bool m_Visible = true;
};

} // namespace Krafter
