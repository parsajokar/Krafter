#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "Krafter/Core/LayerStack.h"

int main(int argc, char** argv);

namespace Krafter {

struct Event;
class Window;
class Renderer;

struct ApplicationSpecification {
    std::string name;
    std::string workingDirectory;
};

class Application {
public:
    Application(const ApplicationSpecification& specification);
    virtual ~Application();

    void PushLayer(Layer* layer);
    void PushOverlay(Layer* layer);

    // The running application. Layers use this to reach application-wide state such
    // as the debug-UI toggle.
    inline static Application& Get()
    {
        return *s_Application;
    }

    // The debug UI (the ImGui overlay built from each layer's RenderImGui) is off
    // by default; only the world layer turns it on, with F3.
    inline void SetDebugUI(bool show)
    {
        m_ShowDebugUI = show;
    }
    inline void ToggleDebugUI()
    {
        m_ShowDebugUI = !m_ShowDebugUI;
    }

protected:
    // Detaches, removes, and deletes a layer. Call only from outside the layer
    // iteration (e.g. via QueueAfterFrame), never from a layer's own callback.
    void RemoveLayer(Layer* layer);

    inline Window& GetWindow()
    {
        return *m_Window;
    }
    inline Renderer& GetRenderer()
    {
        return *m_Renderer;
    }

    // Runs `action` once, after the current frame finishes. Use this to mutate
    // the layer stack (e.g. swapping scenes) from inside a layer's event or
    // update, where the stack is mid-iteration and cannot be changed directly.
    void QueueAfterFrame(std::function<void()> action);

private:
    inline static Application* s_Application = nullptr;

    void Run();

    void OnEvent(Event& event);

    void InitImGui();
    void ShutdownImGui();

    void BeginImGui();
    void EndImGui();

    ApplicationSpecification m_Specification;

    std::unique_ptr<Window> m_Window;
    std::unique_ptr<Renderer> m_Renderer;

    LayerStack m_LayerStack;

    // Whether the debug UI overlay is currently drawn. Off by default; toggled by
    // the world layer with F3.
    bool m_ShowDebugUI = false;

    // Actions deferred from a layer callback, drained at the end of each frame.
    std::vector<std::function<void()>> m_DeferredActions;

    friend int ::main(int argc, char** argv);
};

} // namespace Krafter
