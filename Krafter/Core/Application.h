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

    inline static Application& Get()
    {
        return *s_Application;
    }

    inline void SetDebugUI(bool show)
    {
        m_ShowDebugUI = show;
    }
    inline void ToggleDebugUI()
    {
        m_ShowDebugUI = !m_ShowDebugUI;
    }

protected:
    void RemoveLayer(Layer* layer);

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

    bool m_ShowDebugUI = false;

    std::vector<std::function<void()>> m_DeferredActions;

    friend int ::main(int argc, char** argv);
};

}
