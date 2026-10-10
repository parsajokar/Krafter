#pragma once

#include <memory>
#include <vector>

#include "Krafter/Engine/Layer.h"

namespace Krafter {

class LayerStack {
public:
    using Layers = std::vector<std::unique_ptr<Layer>>;

    void PushLayer(std::unique_ptr<Layer> layer);
    void PushOverlay(std::unique_ptr<Layer> layer);

    std::unique_ptr<Layer> Remove(Layer* layer);

    bool IsEmpty() const
    {
        return m_Layers.empty();
    }

    Layer* Top() const
    {
        return m_Layers.empty() ? nullptr : m_Layers.back().get();
    }

    Layers::iterator begin() { return m_Layers.begin(); }
    Layers::iterator end() { return m_Layers.end(); }
    Layers::const_iterator begin() const { return m_Layers.begin(); }
    Layers::const_iterator end() const { return m_Layers.end(); }

    Layers::reverse_iterator rbegin() { return m_Layers.rbegin(); }
    Layers::reverse_iterator rend() { return m_Layers.rend(); }
    Layers::const_reverse_iterator rbegin() const { return m_Layers.rbegin(); }
    Layers::const_reverse_iterator rend() const { return m_Layers.rend(); }

private:
    Layers m_Layers;
    size_t m_LayerPushIndex = 0;
};

}
