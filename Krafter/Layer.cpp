#include <iostream>

#include "Krafter/Layer.h"

namespace Krafter {

Layer::Layer(std::string_view name)
    : m_Name(name)
{
    m_LayerStack = std::make_shared<LayerStack>();
}

void Layer::PushLayer(Layer* layer)
{
    m_LayerStack->PushLayer(layer);
}

void Layer::PushOverlay(Layer* layer)
{
    m_LayerStack->PushOverlay(layer);
}

void Layer::Attach()
{
    OnAttach();
    std::cout << "[LAYER] " << m_Name << " attached!" << std::endl;

    for (Layer* layer : *m_LayerStack) {
        layer->Attach();
    }
}

void Layer::Detach()
{
    for (auto it = m_LayerStack->rbegin(); it < m_LayerStack->rend(); it++) {
        (*it)->Detach();
        delete *it;
    }

    OnDetach();
    std::cout << "[LAYER] " << m_Name << " detached!" << std::endl;
}

void Layer::Update()
{
    OnUpdate();

    for (Layer* layer : *m_LayerStack) {
        layer->Update();
    }
}

void Layer::Render()
{
    OnRender();

    for (Layer* layer : *m_LayerStack) {
        layer->Render();
    }
}

void Layer::RenderImGui()
{
    OnRenderImGui();

    for (Layer* layer : *m_LayerStack) {
        layer->RenderImGui();
    }
}

} // namespace Krafter
