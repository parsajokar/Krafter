#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "Krafter/Engine/LayerStack.h"

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
    explicit Application(const ApplicationSpecification& specification);
    virtual ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    template <typename T>
    T* PushLayer(std::unique_ptr<T> layer)
    {
        T* raw = layer.get();
        AttachLayer(std::move(layer), false);
        return raw;
    }

    template <typename T>
    T* PushOverlay(std::unique_ptr<T> layer)
    {
        T* raw = layer.get();
        AttachLayer(std::move(layer), true);
        return raw;
    }

protected:
    void SetDebugUIAvailable(bool available)
    {
        m_DebugUIAvailable = available;
        m_ShowDebugUI = m_ShowDebugUI && available;
    }

    void RemoveLayer(Layer* layer);

    void RemoveAllLayers();

    inline Window& GetWindow()
    {
        return *m_Window;
    }
    inline Renderer& GetRenderer()
    {
        return *m_Renderer;
    }

    void QueueAfterFrame(std::function<void()> action);

private:
    void Run();

    void AttachLayer(std::unique_ptr<Layer> layer, bool overlay);

    void OnEvent(Event& event);

    void InitImGui();
    void ShutdownImGui();

    void BeginImGui();
    void EndImGui();

    ApplicationSpecification m_Specification;

    std::unique_ptr<Window> m_Window;
    std::unique_ptr<Renderer> m_Renderer;

    LayerStack m_LayerStack;

    bool m_ShowDebugUI = false;
    bool m_DebugUIAvailable = false;

    std::vector<std::function<void()>> m_DeferredActions;

    friend int ::main(int argc, char** argv);
};

}
