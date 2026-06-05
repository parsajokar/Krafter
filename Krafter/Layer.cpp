#include <iostream>

#include "imgui.h"

#include "Krafter/Layer.h"

namespace Krafter {

Layer::Layer(std::string_view name)
    : m_Name(name)
{
}

void Layer::Attach()
{
    OnAttach();
    std::cout << "[LAYER] " << m_Name << " attached!" << std::endl;
}

void Layer::Detach()
{
    OnDetach();
    std::cout << "[LAYER] " << m_Name << " detached!" << std::endl;
}

void Layer::Update()
{
    OnUpdate();
}

void Layer::Render()
{
    OnRender();
}

void Layer::RenderImGui()
{
    ImGui::Begin(m_Name.c_str());
    OnRenderImGui();
    ImGui::End();
}

void Layer::HandleEvent(Event& event)
{
    OnEvent(event);
}

} // namespace Krafter
