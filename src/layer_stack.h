#pragma once

#include <vector>

#include "layer.h"

namespace Krafter {

class Layer;

class LayerStack {
public:
    void PushLayer(Layer* layer);
    void PushOverlay(Layer* layer);

    inline std::vector<Layer*>::iterator begin()
    {
        return _layers.begin();
    }
    inline std::vector<Layer*>::iterator end()
    {
        return _layers.end();
    }
    inline std::vector<Layer*>::const_iterator begin() const
    {
        return _layers.begin();
    }
    inline std::vector<Layer*>::const_iterator end() const
    {
        return _layers.end();
    }

    inline std::vector<Layer*>::reverse_iterator rbegin()
    {
        return _layers.rbegin();
    }
    inline std::vector<Layer*>::reverse_iterator rend()
    {
        return _layers.rend();
    }
    inline std::vector<Layer*>::const_reverse_iterator rbegin() const
    {
        return _layers.rbegin();
    }
    inline std::vector<Layer*>::const_reverse_iterator rend() const
    {
        return _layers.rend();
    }

private:
    std::vector<Layer*> _layers;
    size_t _layerPushIndex = 0;
};

} // namespace Krafter
