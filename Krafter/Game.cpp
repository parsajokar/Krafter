#include "Krafter/Game.h"
#include "Krafter/InventoryLayer.h"
#include "Krafter/MainMenuLayer.h"
#include "Krafter/PauseMenuLayer.h"
#include "Krafter/UILayer.h"
#include "Krafter/WorldLayer.h"

namespace Krafter {

GameApplication::GameApplication(const ApplicationSpecification& specification)
    : Application(specification)
    , m_UIRenderer(GetWindow())
    , m_UITexture("assets/textures/ui.png")
    , m_Font("assets/textures/font.png")
{
    m_MainMenu = new MainMenuLayer(
        GetWindow(), m_UIRenderer, m_UITexture, m_Font,
        [this](int32_t seed, GameMode mode) { StartGame(seed, mode); });
    PushOverlay(m_MainMenu);
}

GameApplication::~GameApplication()
{
    if (m_PauseMenu != nullptr) {
        RemoveLayer(m_PauseMenu);
    }
    if (m_Inventory != nullptr) {
        RemoveLayer(m_Inventory);
    }
    if (m_UI != nullptr) {
        RemoveLayer(m_UI);
    }
    if (m_World != nullptr) {
        RemoveLayer(m_World);
    }
    if (m_MainMenu != nullptr) {
        RemoveLayer(m_MainMenu);
    }
}

void GameApplication::StartGame(int32_t seed, GameMode mode)
{
    QueueAfterFrame([this, seed, mode]() {
        if (m_MainMenu == nullptr) {
            return;
        }

        m_World = new WorldLayer(
            GetWindow(), GetRenderer(), seed, mode,
            [this]() { PauseGame(); },
            [this]() { OpenInventory(); });
        PushLayer(m_World);

        m_UI = new UILayer(GetWindow(), m_UIRenderer, m_UITexture, m_Font, m_World->GetHotbar());
        PushOverlay(m_UI);

        m_World->BeginPlay();

        RemoveLayer(m_MainMenu);
        m_MainMenu = nullptr;
    });
}

void GameApplication::PauseGame()
{
    QueueAfterFrame([this]() {
        if (m_World == nullptr || m_PauseMenu != nullptr) {
            return;
        }

        m_World->Pause();
        m_PauseMenu = new PauseMenuLayer(
            GetWindow(), m_UIRenderer, m_UITexture, m_Font,
            [this]() { ResumeGame(); },
            [this]() { ReturnToMenu(); });
        PushOverlay(m_PauseMenu);
    });
}

void GameApplication::ResumeGame()
{
    QueueAfterFrame([this]() {
        if (m_PauseMenu == nullptr) {
            return;
        }

        RemoveLayer(m_PauseMenu);
        m_PauseMenu = nullptr;

        if (m_World != nullptr) {
            m_World->Resume();
        }
    });
}

void GameApplication::OpenInventory()
{
    QueueAfterFrame([this]() {
        if (m_World == nullptr || m_Inventory != nullptr) {
            return;
        }

        m_World->SuspendForInventory();
        if (m_UI != nullptr) {
            m_UI->SetVisible(false);
        }
        m_Inventory = new InventoryLayer(
            GetWindow(), m_UIRenderer, m_UITexture, m_Font,
            m_World->GetInventory(), m_World->GetHotbar(), m_World->NearWorkbench(),
            [this]() { CloseInventory(); });
        PushOverlay(m_Inventory);
    });
}

void GameApplication::CloseInventory()
{
    QueueAfterFrame([this]() {
        if (m_Inventory == nullptr) {
            return;
        }

        RemoveLayer(m_Inventory);
        m_Inventory = nullptr;

        if (m_UI != nullptr) {
            m_UI->SetVisible(true);
        }
        if (m_World != nullptr) {
            m_World->Resume();
        }
    });
}

void GameApplication::ReturnToMenu()
{
    QueueAfterFrame([this]() {
        if (m_World == nullptr) {
            return;
        }

        if (m_PauseMenu != nullptr) {
            RemoveLayer(m_PauseMenu);
            m_PauseMenu = nullptr;
        }

        if (m_Inventory != nullptr) {
            RemoveLayer(m_Inventory);
            m_Inventory = nullptr;
        }

        RemoveLayer(m_UI);
        m_UI = nullptr;
        RemoveLayer(m_World);
        m_World = nullptr;

        m_MainMenu = new MainMenuLayer(
            GetWindow(), m_UIRenderer, m_UITexture, m_Font,
            [this](int32_t seed, GameMode mode) { StartGame(seed, mode); });
        PushOverlay(m_MainMenu);
    });
}

}
