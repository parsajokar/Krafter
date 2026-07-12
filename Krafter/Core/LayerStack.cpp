#include <algorithm>

#include "Krafter/Core/LayerStack.h"

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

void LayerStack::Remove(Layer* layer)
{
    const auto it = std::find(m_Layers.begin(), m_Layers.end(), layer);
    if (it == m_Layers.end()) {
        return;
    }

    if (static_cast<size_t>(it - m_Layers.begin()) < m_LayerPushIndex) {
        m_LayerPushIndex--;
    }
    m_Layers.erase(it);
}

}
