#include "Krafter/LayerStack.h"

namespace Krafter {

void LayerStack::PushLayer(Layer* layer)
{
    m_layers.emplace(m_layers.begin() + m_layerPushIndex, layer);
    m_layerPushIndex++;
}

void LayerStack::PushOverlay(Layer* layer)
{
    m_layers.emplace_back(layer);
}

}
