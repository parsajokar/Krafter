#pragma once

#include <cstdint>

#include "Krafter/Core/Application.h"
#include "Krafter/GameMode.h"
#include "Krafter/Renderer/Font.h"
#include "Krafter/Renderer/Texture.h"
#include "Krafter/Renderer/UIRenderer.h"

namespace Krafter {

class MainMenuLayer;
class WorldLayer;
class UILayer;
class PauseMenuLayer;
class InventoryLayer;

// The application composition root: assembles the scene layers it runs and
// wires up the references they share.
class GameApplication : public Application {
public:
    GameApplication(const ApplicationSpecification& specification);

    // Tears down the live layers before the shared UI resources they reference are
    // destroyed (the base destructor would otherwise delete them too late).
    ~GameApplication() override;

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

    // Opens / closes the inventory screen over the running world. OpenInventory is
    // invoked when the player presses 'E' in the world; CloseInventory when the
    // screen is dismissed ('E' or Escape again). Both defer the layer-stack change.
    void OpenInventory();
    void CloseInventory();

    // Tears the world scene down and returns to a fresh main menu. Invoked from the
    // pause menu's "Exit to main menu" button; the swap is deferred to be safe.
    void ReturnToMenu();

    // UI resources shared by every UI layer (menu, pause menu, HUD), owned here so
    // the renderer, sprite sheet, and font are each created once rather than per
    // layer. Declared before the layer pointers so they outlive the layers, which
    // hold references to them.
    UIRenderer m_UIRenderer;
    Texture2D m_UITexture;
    Font m_Font;

    // The active scene layers, each kept so it can be removed on a transition.
    // Exactly the main menu, or the world plus HUD (optionally with the pause menu
    // over them), is live at a time.
    MainMenuLayer* m_MainMenu = nullptr;
    WorldLayer* m_World = nullptr;
    UILayer* m_UI = nullptr;
    PauseMenuLayer* m_PauseMenu = nullptr;
    InventoryLayer* m_Inventory = nullptr;
};

} // namespace Krafter
