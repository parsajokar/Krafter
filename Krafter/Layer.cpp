#include <iostream>

#include "Krafter/Layer.h"

namespace Krafter {

Layer::Layer(std::string_view name)
    : m_name(name)
{
    m_layerStack = std::make_shared<LayerStack>();
}

void Layer::PushLayer(Layer* layer)
{
    m_layerStack->PushLayer(layer);
}

void Layer::PushOverlay(Layer* layer)
{
    m_layerStack->PushOverlay(layer);
}

void Layer::Attach()
{
    OnAttach();
    std::cout << "[LAYER] " << m_name << " attached!" << std::endl;

    for (Layer* layer : *m_layerStack) {
        layer->Attach();
    }
}

void Layer::Detach()
{
    for (auto it = m_layerStack->rbegin(); it < m_layerStack->rend(); it++) {
        (*it)->Detach();
        delete *it;
    }

    OnDetach();
    std::cout << "[LAYER] " << m_name << " detached!" << std::endl;
}

void Layer::Update()
{
    OnUpdate();

    for (Layer* layer : *m_layerStack) {
        layer->Update();
    }
}

void Layer::Render()
{
    OnRender();

    for (Layer* layer : *m_layerStack) {
        layer->Render();
    }
}

void Layer::RenderImGui()
{
    OnRenderImGui();

    for (Layer* layer : *m_layerStack) {
        layer->RenderImGui();
    }
}

} // namespace Krafter
