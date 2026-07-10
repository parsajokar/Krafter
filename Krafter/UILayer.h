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
    UILayer(Window& window, UIRenderer& renderer, Texture2D& uiTexture, Font& font, Hotbar& hotbar);

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

    Texture2D m_BlockTexture;
    Texture2D m_ItemTexture;

    bool m_Visible = true;
};

}
