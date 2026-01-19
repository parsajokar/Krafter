#include "game.h"

int main(int argc, char** argv)
{
    Krafter::Game::Init();
    Krafter::Game::Get()->Run();
    Krafter::Game::Deinit();

    return 0;
}
