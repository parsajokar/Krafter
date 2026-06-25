#pragma once

#include <vector>

#include "Krafter/Core/Layer.h"

namespace Krafter {

class LayerStack {
public:
    void PushLayer(Layer* layer);
    void PushOverlay(Layer* layer);

    inline std::vector<Layer*>::iterator begin()
    {
        return m_Layers.begin();
    }
    inline std::vector<Layer*>::iterator end()
    {
        return m_Layers.end();
    }
    inline std::vector<Layer*>::const_iterator begin() const
    {
        return m_Layers.begin();
    }
    inline std::vector<Layer*>::const_iterator end() const
    {
        return m_Layers.end();
    }

    inline std::vector<Layer*>::reverse_iterator rbegin()
    {
        return m_Layers.rbegin();
    }
    inline std::vector<Layer*>::reverse_iterator rend()
    {
        return m_Layers.rend();
    }
    inline std::vector<Layer*>::const_reverse_iterator rbegin() const
    {
        return m_Layers.rbegin();
    }
    inline std::vector<Layer*>::const_reverse_iterator rend() const
    {
        return m_Layers.rend();
    }

private:
    std::vector<Layer*> m_Layers;
    size_t m_LayerPushIndex = 0;
};

} // namespace Krafter
