#pragma once

#include <functional>

#include "glm/glm.hpp"

#include "Krafter/Core/Layer.h"
#include "Krafter/Renderer/Font.h"
#include "Krafter/Renderer/Texture.h"
#include "Krafter/Renderer/UIRenderer.h"

namespace Krafter {

class Window;

// The in-game pause screen, shown over the world (and HUD) when Escape is pressed
// in either game mode. Dims the scene and offers a single "Exit to main menu"
// button; Escape again resumes play. The two callbacks run the resume and the
// teardown-to-menu, leaving the actual layer-stack changes to the owner.
class PauseMenuLayer : public Layer {
public:
    // The UI renderer, sprite sheet (ui.png), and font are owned by the
    // application and shared across the UI layers, so the menu only borrows them.
    PauseMenuLayer(
        Window& window, UIRenderer& renderer, Texture2D& uiTexture, Font& font,
        std::function<void()> onResume, std::function<void()> onExitToMenu);

private:
    void OnRender() override;
    void OnEvent(Event& event) override;

    // The "Resume" and "Exit to main menu" button rectangles (xy = top-left,
    // zw = size), stacked and centred in the window and recomputed each use so
    // they track window resizes.
    glm::vec4 ResumeRect() const;
    glm::vec4 ExitRect() const;

    // The shared width of both buttons: wide enough for the longer label.
    float ButtonWidth() const;

    Window& m_Window;
    std::function<void()> m_OnResume;
    std::function<void()> m_OnExitToMenu;

    UIRenderer& m_Renderer;
    Texture2D& m_UITexture;
    Font& m_Font;

    glm::vec2 m_Cursor = glm::vec2(0.0f);
};

} // namespace Krafter
