#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

#include "glm/glm.hpp"

#include "Krafter/Core/Layer.h"
#include "Krafter/GameMode.h"
#include "Krafter/Renderer/Font.h"
#include "Krafter/Renderer/Texture.h"
#include "Krafter/Renderer/UIRenderer.h"

namespace Krafter {

class Window;

// The main menu, shown over the world on startup. A seed text field, a "Survive"
// and a "Spectate" button (each runs the supplied callback with the chosen seed
// and game mode, then dismisses the menu), and an "Exit" button that closes the
// window.
class MainMenuLayer : public Layer {
public:
    MainMenuLayer(Window& window, std::function<void(int32_t, GameMode)> onPlay);

private:
    void OnAttach() override;
    void OnRender() override;
    void OnEvent(Event& event) override;

    // The seed field, Survive button, Spectate button, and Exit button rectangles
    // (xy = top-left, zw = size), stacked and centred in the window, recomputed
    // each use so they track window resizes.
    glm::vec4 SeedRect() const;
    glm::vec4 SurviveRect() const;
    glm::vec4 SpectateRect() const;
    glm::vec4 ExitRect() const;
    static bool Contains(const glm::vec4& rect, const glm::vec2& point);

    // Turns the typed seed text into a generation seed: an integer is used as
    // typed, any other text is hashed, and an empty field gets a time-based seed.
    int32_t SeedFromText() const;

    // Dismisses the menu and starts the world with the current seed in `mode`.
    void Play(GameMode mode);

    // Draws a button: the slot sprite stretched to the rect, a hover highlight,
    // and the label centred inside.
    void DrawButton(const glm::vec4& rect, std::string_view label);

    // Draws a sprite from the UI texture, addressed from its bottom-left like the
    // HUD does, stretched to fill the destination rectangle.
    void DrawSprite(
        const glm::vec2& spritePos, const glm::vec2& spriteSize,
        const glm::vec2& position, const glm::vec2& size, float opacity = 1.0f);

    // Draws a sprite as a horizontal 3-slice: the outer thirds (the rounded
    // caps) keep their aspect ratio while only the middle third stretches, so a
    // small slot sprite makes a clean wide button or field.
    void DrawSlicedSprite(
        const glm::vec2& spritePos, const glm::vec2& spriteSize,
        const glm::vec2& position, const glm::vec2& size, float opacity = 1.0f);

    Window& m_Window;
    std::function<void(int32_t, GameMode)> m_OnPlay;

    UIRenderer m_Renderer;
    Texture2D m_UITexture;
    Texture2D m_Background;
    Font m_Font;

    // Cleared once Play runs; the menu then renders nothing and lets all input
    // fall through to the world beneath it.
    bool m_Active = true;
    glm::vec2 m_Cursor = glm::vec2(0.0f);

    std::string m_Seed;
    bool m_SeedFocused = false;
};

} // namespace Krafter
