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

    // Actions deferred from a layer callback, drained at the end of each frame.
    std::vector<std::function<void()>> m_DeferredActions;

    friend int ::main(int argc, char** argv);
};

} // namespace Krafter
