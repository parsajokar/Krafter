#pragma once

#include <cstdint>

#include "Krafter/Core/Application.h"
#include "Krafter/GameMode.h"

namespace Krafter {

class MainMenuLayer;
class WorldLayer;
class UILayer;
class PauseMenuLayer;

// The application composition root: assembles the scene layers it runs and
// wires up the references they share.
class GameApplication : public Application {
public:
    GameApplication(const ApplicationSpecification& specification);

private:
    // Builds the world scene for `seed` in the chosen `mode` and enters it.
    // Invoked when "Survive" or "Spectate" is pressed on the main menu; the actual
    // layer swap is deferred to a safe point.
    void StartGame(int32_t seed, GameMode mode);

    // Brings up / dismisses the pause menu over the running world. PauseGame is
    // invoked when the player presses Escape in the world; ResumeGame when the
    // pause menu is dismissed (Escape again). Both defer the layer-stack change.
    void PauseGame();
    void ResumeGame();

    // Tears the world scene down and returns to a fresh main menu. Invoked from the
    // pause menu's "Exit to main menu" button; the swap is deferred to be safe.
    void ReturnToMenu();

    // The active scene layers, each kept so it can be removed on a transition.
    // Exactly the main menu, or the world plus HUD (optionally with the pause menu
    // over them), is live at a time.
    MainMenuLayer* m_MainMenu = nullptr;
    WorldLayer* m_World = nullptr;
    UILayer* m_UI = nullptr;
    PauseMenuLayer* m_PauseMenu = nullptr;
};

} // namespace Krafter
