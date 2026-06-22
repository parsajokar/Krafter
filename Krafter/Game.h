#pragma once

#include "Krafter/Application.h"
#include "Krafter/Camera.h"
#include "Krafter/Hotbar.h"
#include "Krafter/Layer.h"
#include "Krafter/Sky.h"
#include "Krafter/World/World.h"

namespace Krafter {

class Window;
class Renderer;

class GameLayer : public Layer {
public:
    GameLayer(Window& window, Renderer& renderer, Hotbar& hotbar);

private:
    void OnUpdate() override;
    void OnRender() override;
    void OnEvent(Event& event) override;
    void OnRenderImGui() override;

    // How far the player can reach to break, place, or target a block.
    static constexpr float k_Reach = 8.0f;

    Window& m_Window;
    Renderer& m_Renderer;
    Hotbar& m_Hotbar;

    Sky m_Sky;
    Camera m_Camera;
    World m_World;

    // The block the camera is currently looking at, for the highlight outline.
    bool m_HasTarget = false;
    glm::ivec3 m_TargetBlock = glm::ivec3(0);
};

class GameApplication : public Application {
public:
    GameApplication(const ApplicationSpecification& specification);

private:
    Hotbar m_Hotbar;
};

} // namespace Krafter
