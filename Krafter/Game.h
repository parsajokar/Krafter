#pragma once

#include "Krafter/Application.h"
#include "Krafter/Layer.h"
#include "Krafter/World/ChunkManager.h"

namespace Krafter {

class Game : public Layer {
public:
    Game();
    ~Game() = default;

    void OnUpdate() override;

private:
    ChunkManager* m_ChunkManager;
};

class GameApplication : public Application {
public:
    GameApplication(const ApplicationSpec& spec);
};

} // namespace Krafter
