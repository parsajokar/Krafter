#pragma once

#include <string>
#include <string_view>

namespace Krafter {

struct Event;

class Layer {
public:
    Layer(std::string_view name);
    virtual ~Layer() = default;

    void Attach();
    void Detach();
    void Update();
    void Render();
    void RenderImGui();
    void HandleEvent(Event& event);

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
    virtual void OnEvent(Event& event) { }

    std::string m_Name;
};

} // namespace Krafter
