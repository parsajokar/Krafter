#include "Krafter/UILayer.h"
#include "Krafter/Window.h"

namespace Krafter {

UILayer::UILayer(Window& window)
    : Layer("UI")
    , m_Window(window)
    , m_Renderer(window)
{
}

void UILayer::OnRender()
{
    m_Renderer.Begin();
    DrawCrosshair();
    m_Renderer.End();
}

void UILayer::DrawCrosshair()
{
    constexpr float k_Length = 20.0f;
    constexpr float k_Thickness = 2.0f;
    const glm::vec4 k_Color = glm::vec4(1.0f);

    const glm::vec2 center = glm::vec2(m_Window.GetSize()) * 0.5f;

    m_Renderer.DrawQuad(
        center - glm::vec2(k_Length, k_Thickness) * 0.5f,
        glm::vec2(k_Length, k_Thickness), k_Color);
    m_Renderer.DrawQuad(
        center - glm::vec2(k_Thickness, k_Length) * 0.5f,
        glm::vec2(k_Thickness, k_Length), k_Color);
}

} // namespace Krafter
