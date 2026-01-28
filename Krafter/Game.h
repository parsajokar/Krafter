#pragma once

#include "Krafter/Application.h"
#include "Krafter/Layer.h"
#include "Krafter/World/ChunkManager.h"

namespace Krafter {

class GameLayer : public Layer {
public:
    GameLayer();

    void OnUpdate() override;
    void OnRender() override;
    void OnRenderImGui() override;

private:
    Camera m_Camera;
    ChunkManager m_ChunkManager;
};

class GameApplication : public Application {
public:
    GameApplication(const ApplicationSpecification& specification);
};

} // namespace Krafter
