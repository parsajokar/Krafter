#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "Krafter/GameMode.h"
#include "Krafter/Renderer/Texture.h"
#include "Krafter/UIScreen.h"

namespace Krafter {

class Window;
class UIRenderer;
class Font;

class MainMenuLayer : public UIScreen {
public:
    MainMenuLayer(
        Window& window, UIRenderer& renderer, Texture2D& uiTexture, Font& font,
        std::function<void(int32_t, GameMode)> onPlay);

private:
    void OnAttach() override;
    void OnRender() override;
    void OnEvent(Event& event) override;

    glm::vec4 SeedRect() const;
    glm::vec4 SurviveRect() const;
    glm::vec4 SpectateRect() const;
    glm::vec4 ExitRect() const;

    int32_t SeedFromText() const;

    void Play(GameMode mode);

    std::function<void(int32_t, GameMode)> m_OnPlay;

    Texture2D m_Background;

    bool m_Active = true;

    std::string m_Seed;
    bool m_SeedFocused = false;
};

}
