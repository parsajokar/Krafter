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

// The main menu, shown over the world on startup. A seed text field, a "Survive"
// and a "Spectate" button (each runs the supplied callback with the chosen seed
// and game mode, then dismisses the menu), and an "Exit" button that closes the
// window.
class MainMenuLayer : public UIScreen {
public:
    // The UI renderer, sprite sheet (ui.png), and font are owned by the
    // application and shared across the UI layers, so the menu only borrows them.
    MainMenuLayer(
        Window& window, UIRenderer& renderer, Texture2D& uiTexture, Font& font,
        std::function<void(int32_t, GameMode)> onPlay);

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

    // Turns the typed seed text into a generation seed: an integer is used as
    // typed, any other text is hashed, and an empty field gets a time-based seed.
    int32_t SeedFromText() const;

    // Dismisses the menu and starts the world with the current seed in `mode`.
    void Play(GameMode mode);

    std::function<void(int32_t, GameMode)> m_OnPlay;

    // The backdrop image is the menu's own, not shared.
    Texture2D m_Background;

    // Cleared once Play runs; the menu then renders nothing and lets all input
    // fall through to the world beneath it.
    bool m_Active = true;

    std::string m_Seed;
    bool m_SeedFocused = false;
};

} // namespace Krafter
