#pragma once

#include <functional>
#include <string_view>

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
    PauseMenuLayer(Window& window, std::function<void()> onResume, std::function<void()> onExitToMenu);

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

    static bool Contains(const glm::vec4& rect, const glm::vec2& point);

    // Draws a button: the slot sprite stretched to the rect, a hover highlight,
    // and the label centred inside.
    void DrawButton(const glm::vec4& rect, std::string_view label);

    // Draws a sprite from the UI texture, addressed from its bottom-left like the
    // HUD does, stretched to fill the destination rectangle.
    void DrawSprite(
        const glm::vec2& spritePos, const glm::vec2& spriteSize,
        const glm::vec2& position, const glm::vec2& size, float opacity = 1.0f);

    // Draws a sprite as a horizontal 3-slice so a small slot sprite makes a clean
    // wide button: the rounded caps keep their aspect, only the middle stretches.
    void DrawSlicedSprite(
        const glm::vec2& spritePos, const glm::vec2& spriteSize,
        const glm::vec2& position, const glm::vec2& size, float opacity = 1.0f);

    Window& m_Window;
    std::function<void()> m_OnResume;
    std::function<void()> m_OnExitToMenu;

    UIRenderer m_Renderer;
    Texture2D m_UITexture;
    Font m_Font;

    glm::vec2 m_Cursor = glm::vec2(0.0f);
};

} // namespace Krafter
