#pragma once

#include <cstdint>

#include "Krafter/Core/Application.h"

namespace Krafter {

class MainMenuLayer;
class WorldLayer;
class UILayer;

// The application composition root: assembles the scene layers it runs and
// wires up the references they share.
class GameApplication : public Application {
public:
    GameApplication(const ApplicationSpecification& specification);

private:
    // Builds the world scene for `seed` and enters it. Invoked when "Play!" is
    // pressed on the main menu; the actual layer swap is deferred to a safe point.
    void StartGame(int32_t seed);

    // Tears the world scene down and returns to a fresh main menu. Invoked when
    // the player presses Escape in the world; the swap is deferred to be safe.
    void ReturnToMenu();

    // The active scene layers, each kept so it can be removed on a transition.
    // Exactly the main menu, or exactly the world plus HUD, is live at a time.
    MainMenuLayer* m_MainMenu = nullptr;
    WorldLayer* m_World = nullptr;
    UILayer* m_UI = nullptr;
};

} // namespace Krafter
