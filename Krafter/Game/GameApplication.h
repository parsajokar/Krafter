#pragma once

#include <cstdint>
#include <memory>
#include <optional>

#include "Krafter/Engine/Application.h"
#include "Krafter/Engine/Graphics/Texture.h"
#include "Krafter/Gameplay/GameMode.h"
#include "Krafter/Rendering/Font.h"
#include "Krafter/Rendering/UIRenderer.h"

namespace Krafter {

class MainMenuLayer;
class WorldLayer;
class HudLayer;
class PauseMenuLayer;
class InventoryLayer;

class GameApplication : public Application {
public:
    explicit GameApplication(const ApplicationSpecification& specification);

    ~GameApplication() override;

private:
    struct Session {
        WorldLayer* world = nullptr;
        HudLayer* hud = nullptr;
        Layer* modal = nullptr;
    };

    void ShowMainMenu();
    void StartGame(int32_t seed, GameMode mode);
    void ReturnToMenu();

    void PauseGame();
    void OpenInventory();

    void OpenModal(std::unique_ptr<Layer> screen);
    void CloseModal();

    UIRenderer m_UIRenderer;
    Texture2D m_UITexture;
    Font m_Font;

    Texture2D m_BlockTexture;
    Texture2D m_ItemTexture;

    MainMenuLayer* m_MainMenu = nullptr;
    std::optional<Session> m_Session;
};

}
