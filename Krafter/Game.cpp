#include "Krafter/Game.h"
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
    // Start on the main menu alone; the world is not built until a play mode is
    // chosen ("Survive" or "Spectate").
    m_MainMenu = new MainMenuLayer(
        GetWindow(), m_UIRenderer, m_UITexture, m_Font,
        [this](int32_t seed, GameMode mode) { StartGame(seed, mode); });
    PushOverlay(m_MainMenu);
}

GameApplication::~GameApplication()
{
    // Remove every live layer now, while the shared UI resources are still alive;
    // the base destructor only runs after our members (those resources) are gone.
    // Order mirrors the scene transitions: pause menu, HUD, world, then menu.
    if (m_PauseMenu != nullptr) {
        RemoveLayer(m_PauseMenu);
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
    // Defer the layer swap: this runs from the menu's click handler, while the
    // layer stack is being iterated, so the world cannot be pushed in place.
    QueueAfterFrame([this, seed, mode]() {
        // Ignore a duplicate trigger from the same frame: the menu is gone once
        // the first swap ran.
        if (m_MainMenu == nullptr) {
            return;
        }

        m_World = new WorldLayer(GetWindow(), GetRenderer(), seed, mode, [this]() { PauseGame(); });
        PushLayer(m_World);

        // The HUD shares the world's player hotbar. The overlay is pushed after
        // the world layer and so is destroyed before it, keeping the reference
        // valid.
        m_UI = new UILayer(GetWindow(), m_UIRenderer, m_UITexture, m_World->GetHotbar());
        PushOverlay(m_UI);

        m_World->BeginPlay();

        // The menu has done its job; tear it down now rather than leaving it
        // dormant in the stack until shutdown.
        RemoveLayer(m_MainMenu);
        m_MainMenu = nullptr;
    });
}

void GameApplication::PauseGame()
{
    // Defer the layer push: this runs from the world layer's event handler while
    // the stack is mid-iteration.
    QueueAfterFrame([this]() {
        // Ignore if there's no world, or the menu is already up (a repeat Escape).
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
    // Defer the layer removal: this runs from the pause menu's event handler while
    // the stack is mid-iteration.
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

void GameApplication::ReturnToMenu()
{
    // Defer the swap for the same reason as StartGame: this runs from the pause
    // menu's event handler while the stack is mid-iteration.
    QueueAfterFrame([this]() {
        // Ignore a duplicate trigger from the same frame: the world is gone once
        // the first swap ran.
        if (m_World == nullptr) {
            return;
        }

        // The pause menu sits on top; remove it first if it's still up.
        if (m_PauseMenu != nullptr) {
            RemoveLayer(m_PauseMenu);
            m_PauseMenu = nullptr;
        }

        // Remove the HUD before the world: the HUD holds a reference into the
        // world's player, so the world must outlive it.
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

} // namespace Krafter
