#pragma once

#include "Krafter/Engine/Graphics/Texture.h"
#include "Krafter/Gameplay/Hotbar.h"
#include "Krafter/UI/UIScreen.h"

namespace Krafter {

class Window;
class UIRenderer;
class Font;

class HudLayer : public UIScreen {
public:
    HudLayer(Window& window, UIRenderer& renderer, Texture2D& uiTexture, Font& font,
        const Texture2D& blockTexture, const Texture2D& itemTexture, Hotbar& hotbar);

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

    const Texture2D& m_BlockTexture;
    const Texture2D& m_ItemTexture;

    bool m_Visible = true;
};

}
