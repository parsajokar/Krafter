#pragma once

#include <functional>

#include "glm/glm.hpp"

#include "Krafter/UIScreen.h"

namespace Krafter {

class Window;
class UIRenderer;
class Texture2D;
class Font;

class PauseMenuLayer : public UIScreen {
public:
    PauseMenuLayer(
        Window& window, UIRenderer& renderer, Texture2D& uiTexture, Font& font,
        std::function<void()> onResume, std::function<void()> onExitToMenu);

private:
    void OnRender() override;
    void OnEvent(Event& event) override;

    glm::vec4 ResumeRect() const;
    glm::vec4 ExitRect() const;

    float ButtonWidth() const;

    std::function<void()> m_OnResume;
    std::function<void()> m_OnExitToMenu;
};

}
