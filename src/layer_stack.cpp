#include <algorithm>

#include "layer_stack.h"

namespace Krafter {

void LayerStack::PushLayer(Layer* layer)
{
    _layers.emplace(_layers.begin() + _layerPushIndex, layer);
    _layerPushIndex++;
}

void LayerStack::PushOverlay(Layer* layer)
{
    _layers.emplace_back(layer);
}

}
