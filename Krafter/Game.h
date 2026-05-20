#pragma once

#include "Krafter/Application.h"
#include "Krafter/Layer.h"
#include "Krafter/World/World.h"

namespace Krafter {

class GameLayer : public Layer {
public:
    GameLayer();

private:
    void OnUpdate() override;
    void OnRender() override;
    void OnRenderImGui() override;

    Camera m_Camera;
    World m_World;
};

class GameApplication : public Application {
public:
    GameApplication(const ApplicationSpecification& specification);
};

} // namespace Krafter
