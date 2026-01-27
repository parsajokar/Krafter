#include "Krafter/Game.h"
#include "Krafter/Window.h"

namespace Krafter {

Game::Game()
    : Layer("Game")
{
    PushLayer(m_ChunkManager = new ChunkManager);
}

void Game::OnUpdate()
{
    if (Window::Get()->IsKeyDown(Key::k_Escape)) {
        Window::Get()->Close();
    }
}

GameApplication::GameApplication(const ApplicationSpec& spec)
    : Application(spec)
{
    PushLayer(new Game());
}

} // namespace Krafter
