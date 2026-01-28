#pragma once

#include <string>
#include <string_view>

namespace Krafter {

class Layer {
public:
    Layer(std::string_view name);
    virtual ~Layer() = default;

    void Attach();
    void Detach();
    void Update();
    void Render();
    void RenderImGui();

    inline const std::string& GetName() const
    {
        return m_Name;
    }

private:
    virtual void OnAttach() { }
    virtual void OnDetach() { }
    virtual void OnUpdate() { }
    virtual void OnRender() { }
    virtual void OnRenderImGui() { }

    std::string m_Name;
};

} // namespace Krafter
