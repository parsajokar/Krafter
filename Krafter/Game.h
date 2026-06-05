#pragma once

#include "Krafter/Application.h"
#include "Krafter/Camera.h"
#include "Krafter/Layer.h"
#include "Krafter/Sky.h"
#include "Krafter/World/World.h"

namespace Krafter {

class Window;
class Renderer;

class GameLayer : public Layer {
public:
    GameLayer(Window& window, Renderer& renderer);

private:
    void OnUpdate() override;
    void OnRender() override;
    void OnEvent(Event& event) override;
    void OnRenderImGui() override;

    Window& m_Window;
    Renderer& m_Renderer;

    Sky m_Sky;
    Camera m_Camera;
    World m_World;
};

class GameApplication : public Application {
public:
    GameApplication(const ApplicationSpecification& specification);
};

} // namespace Krafter
