#include "game.h"
#include "window.h"

namespace Krafter {

Game::Game()
    : Layer("Game")
{
    PushLayer(_chunkManager = new ChunkManager);
}

void Game::OnUpdate()
{
    if (Window::Get()->IsKeyDown(Key::ESCAPE)) {
        Window::Get()->Close();
    }
}

GameApplication::GameApplication(const ApplicationSpec& spec)
    : Application(spec)
{
    PushLayer(new Game());
}

} // namespace Krafter
