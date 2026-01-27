#pragma once

#include <vector>

#include "Krafter/Layer.h"

namespace Krafter {

class Layer;

class LayerStack {
public:
    void PushLayer(Layer* layer);
    void PushOverlay(Layer* layer);

    inline std::vector<Layer*>::iterator begin()
    {
        return m_layers.begin();
    }
    inline std::vector<Layer*>::iterator end()
    {
        return m_layers.end();
    }
    inline std::vector<Layer*>::const_iterator begin() const
    {
        return m_layers.begin();
    }
    inline std::vector<Layer*>::const_iterator end() const
    {
        return m_layers.end();
    }

    inline std::vector<Layer*>::reverse_iterator rbegin()
    {
        return m_layers.rbegin();
    }
    inline std::vector<Layer*>::reverse_iterator rend()
    {
        return m_layers.rend();
    }
    inline std::vector<Layer*>::const_reverse_iterator rbegin() const
    {
        return m_layers.rbegin();
    }
    inline std::vector<Layer*>::const_reverse_iterator rend() const
    {
        return m_layers.rend();
    }

private:
    std::vector<Layer*> m_layers;
    size_t m_layerPushIndex = 0;
};

} // namespace Krafter
