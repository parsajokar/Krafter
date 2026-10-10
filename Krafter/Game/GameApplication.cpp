#include "Krafter/Game/GameApplication.h"
#include "Krafter/Game/WorldLayer.h"
#include "Krafter/UI/HudLayer.h"
#include "Krafter/UI/InventoryLayer.h"
#include "Krafter/UI/MainMenuLayer.h"
#include "Krafter/UI/PauseMenuLayer.h"

namespace Krafter {

GameApplication::GameApplication(const ApplicationSpecification& specification)
    : Application(specification)
    , m_UIRenderer(GetRenderer(), GetWindow())
    , m_UITexture(GetRenderer(), "assets/textures/ui.png")
    , m_Font(GetRenderer(), "assets/textures/font.png")
    , m_BlockTexture(GetRenderer(), "assets/textures/blocks.png")
    , m_ItemTexture(GetRenderer(), "assets/textures/items.png")
{
    ShowMainMenu();
}

GameApplication::~GameApplication()
{
    RemoveAllLayers();
}

void GameApplication::ShowMainMenu()
{
    m_MainMenu = PushOverlay(std::make_unique<MainMenuLayer>(
        GetRenderer(), GetWindow(), m_UIRenderer, m_UITexture, m_Font,
        [this](int32_t seed, GameMode mode) { StartGame(seed, mode); }));
}

void GameApplication::StartGame(int32_t seed, GameMode mode)
{
    QueueAfterFrame([this, seed, mode]() {
        if (m_MainMenu == nullptr) {
            return;
        }

        Session session;
        session.world = PushLayer(std::make_unique<WorldLayer>(
            GetWindow(), GetRenderer(), seed, mode,
            [this]() { PauseGame(); },
            [this]() { OpenInventory(); }));
        session.hud = PushOverlay(std::make_unique<HudLayer>(
            GetWindow(), m_UIRenderer, m_UITexture, m_Font, m_BlockTexture, m_ItemTexture,
            session.world->GetHotbar()));
        session.world->BeginPlay();
        m_Session = session;
        SetDebugUIAvailable(true);

        RemoveLayer(m_MainMenu);
        m_MainMenu = nullptr;
    });
}

void GameApplication::ReturnToMenu()
{
    QueueAfterFrame([this]() {
        if (!m_Session) {
            return;
        }

        RemoveLayer(m_Session->modal);
        RemoveLayer(m_Session->hud);
        RemoveLayer(m_Session->world);
        m_Session.reset();
        SetDebugUIAvailable(false);

        ShowMainMenu();
    });
}

void GameApplication::PauseGame()
{
    QueueAfterFrame([this]() {
        if (!m_Session || m_Session->modal != nullptr) {
            return;
        }

        m_Session->world->Pause();
        OpenModal(std::make_unique<PauseMenuLayer>(
            GetWindow(), m_UIRenderer, m_UITexture, m_Font,
            [this]() { CloseModal(); },
            [this]() { ReturnToMenu(); }));
    });
}

void GameApplication::OpenInventory()
{
    QueueAfterFrame([this]() {
        if (!m_Session || m_Session->modal != nullptr) {
            return;
        }

        WorldLayer& world = *m_Session->world;
        world.SuspendForInventory();
        m_Session->hud->SetVisible(false);
        OpenModal(std::make_unique<InventoryLayer>(
            GetWindow(), m_UIRenderer, m_UITexture, m_Font, m_BlockTexture, m_ItemTexture,
            world.GetInventory(), world.GetHotbar(), world.GetNearbyStations(),
            [&world](const Item& item) { world.DropItem(item); },
            [this]() { CloseModal(); }));
    });
}

void GameApplication::OpenModal(std::unique_ptr<Layer> screen)
{
    m_Session->modal = PushOverlay(std::move(screen));
}

void GameApplication::CloseModal()
{
    QueueAfterFrame([this]() {
        if (!m_Session || m_Session->modal == nullptr) {
            return;
        }

        RemoveLayer(m_Session->modal);
        m_Session->modal = nullptr;
        m_Session->hud->SetVisible(true);
        m_Session->world->Resume();
    });
}

}
