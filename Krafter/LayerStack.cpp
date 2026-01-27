#include "Krafter/LayerStack.h"

namespace Krafter {

void LayerStack::PushLayer(Layer* layer)
{
    m_Layers.emplace(m_Layers.begin() + m_LayerPushIndex, layer);
    m_LayerPushIndex++;
}

void LayerStack::PushOverlay(Layer* layer)
{
    m_Layers.emplace_back(layer);
}

}
