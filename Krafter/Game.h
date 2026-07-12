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

class GameApplication : public Application {
public:
    GameApplication(const ApplicationSpecification& specification);

    ~GameApplication() override;

private:
    void StartGame(int32_t seed, GameMode mode);

    void PauseGame();
    void ResumeGame();

    void OpenInventory();
    void CloseInventory();

    void ReturnToMenu();

    UIRenderer m_UIRenderer;
    Texture2D m_UITexture;
    Font m_Font;

    MainMenuLayer* m_MainMenu = nullptr;
    WorldLayer* m_World = nullptr;
    UILayer* m_UI = nullptr;
    PauseMenuLayer* m_PauseMenu = nullptr;
    InventoryLayer* m_Inventory = nullptr;
};

}
