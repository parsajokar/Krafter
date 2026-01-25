#include <iostream>

#include "layer.h"

namespace Krafter {

Layer::Layer(std::string_view name)
    : _name(name)
{
    _layerStack = std::make_shared<LayerStack>();
}

void Layer::PushLayer(Layer* layer)
{
    _layerStack->PushLayer(layer);
}

void Layer::PushOverlay(Layer* layer)
{
    _layerStack->PushOverlay(layer);
}

void Layer::Attach()
{
    OnAttach();
    std::cout << "[LAYER] " << _name << " attached!" << std::endl;

    for (Layer* layer : *_layerStack) {
        layer->Attach();
    }
}

void Layer::Detach()
{
    for (auto it = _layerStack->rbegin(); it < _layerStack->rend(); it++) {
        (*it)->Detach();
        delete *it;
    }

    OnDetach();
    std::cout << "[LAYER] " << _name << " detached!" << std::endl;
}

void Layer::Update()
{
    OnUpdate();

    for (Layer* layer : *_layerStack) {
        layer->Update();
    }
}

void Layer::Render()
{
    OnRender();

    for (Layer* layer : *_layerStack) {
        layer->Render();
    }
}

void Layer::RenderImGui()
{
    OnRenderImGui();

    for (Layer* layer : *_layerStack) {
        layer->RenderImGui();
    }
}

} // namespace Krafter
