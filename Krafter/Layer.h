#pragma once

#include <memory>
#include <string>
#include <string_view>

#include "Krafter/LayerStack.h"

namespace Krafter {

class LayerStack;

class Layer {
public:
    Layer(std::string_view name);
    virtual ~Layer() = default;

    void PushLayer(Layer* layer);
    void PushOverlay(Layer* layer);

    void Attach();
    void Detach();
    void Update();
    void Render();
    void RenderImGui();

    virtual void OnAttach() { }
    virtual void OnDetach() { }
    virtual void OnUpdate() { }
    virtual void OnRender() { }
    virtual void OnRenderImGui() { }

    inline const std::string& GetName() const
    {
        return m_Name;
    }

private:
    std::string m_Name;
    std::shared_ptr<LayerStack> m_LayerStack;
};

} // namespace Krafter
