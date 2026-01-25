#pragma once

#include "application.h"
#include "layer.h"
#include "world/chunk_manager.h"

namespace Krafter {

class Game : public Layer {
public:
    Game();
    ~Game() = default;

    void OnUpdate() override;

private:
    ChunkManager* _chunkManager;
};

class GameApplication : public Application {
public:
    GameApplication(const ApplicationSpec& spec);
};

} // namespace Krafter
