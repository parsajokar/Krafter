#include "Krafter/Game.h"
#include "Krafter/MainMenuLayer.h"
#include "Krafter/UILayer.h"
#include "Krafter/WorldLayer.h"

namespace Krafter {

GameApplication::GameApplication(const ApplicationSpecification& specification)
    : Application(specification)
{
    // Start on the main menu alone; the world is not built until "Play!".
    m_MainMenu = new MainMenuLayer(GetWindow(), [this](int32_t seed) { StartGame(seed); });
    PushOverlay(m_MainMenu);
}

void GameApplication::StartGame(int32_t seed)
{
    // Defer the layer swap: this runs from the menu's click handler, while the
    // layer stack is being iterated, so the world cannot be pushed in place.
    QueueAfterFrame([this, seed]() {
        // Ignore a duplicate trigger from the same frame: the menu is gone once
        // the first swap ran.
        if (m_MainMenu == nullptr) {
            return;
        }

        m_World = new WorldLayer(GetWindow(), GetRenderer(), seed, [this]() { ReturnToMenu(); });
        PushLayer(m_World);

        // The HUD shares the world's player hotbar. The overlay is pushed after
        // the world layer and so is destroyed before it, keeping the reference
        // valid.
        m_UI = new UILayer(GetWindow(), m_World->GetHotbar());
        PushOverlay(m_UI);

        m_World->BeginPlay();

        // The menu has done its job; tear it down now rather than leaving it
        // dormant in the stack until shutdown.
        RemoveLayer(m_MainMenu);
        m_MainMenu = nullptr;
    });
}

void GameApplication::ReturnToMenu()
{
    // Defer the swap for the same reason as StartGame: this runs from the world
    // layer's event handler while the stack is mid-iteration.
    QueueAfterFrame([this]() {
        // Ignore a duplicate trigger from the same frame: the world is gone once
        // the first swap ran.
        if (m_World == nullptr) {
            return;
        }

        // Remove the HUD before the world: the HUD holds a reference into the
        // world's player, so the world must outlive it.
        RemoveLayer(m_UI);
        m_UI = nullptr;
        RemoveLayer(m_World);
        m_World = nullptr;

        m_MainMenu = new MainMenuLayer(GetWindow(), [this](int32_t seed) { StartGame(seed); });
        PushOverlay(m_MainMenu);
    });
}

} // namespace Krafter
