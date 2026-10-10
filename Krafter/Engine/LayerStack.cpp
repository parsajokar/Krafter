#include <algorithm>

#include "Krafter/Engine/LayerStack.h"

namespace Krafter {

void LayerStack::PushLayer(std::unique_ptr<Layer> layer)
{
    m_Layers.emplace(m_Layers.begin() + m_LayerPushIndex, std::move(layer));
    m_LayerPushIndex++;
}

void LayerStack::PushOverlay(std::unique_ptr<Layer> layer)
{
    m_Layers.emplace_back(std::move(layer));
}

std::unique_ptr<Layer> LayerStack::Remove(Layer* layer)
{
    const auto it = std::find_if(m_Layers.begin(), m_Layers.end(),
        [layer](const std::unique_ptr<Layer>& entry) { return entry.get() == layer; });
    if (it == m_Layers.end()) {
        return nullptr;
    }

    if (static_cast<size_t>(it - m_Layers.begin()) < m_LayerPushIndex) {
        m_LayerPushIndex--;
    }
    std::unique_ptr<Layer> removed = std::move(*it);
    m_Layers.erase(it);
    return removed;
}

}
