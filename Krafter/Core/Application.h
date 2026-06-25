#pragma once

#include <memory>
#include <string>

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
    inline Window& GetWindow()
    {
        return *m_Window;
    }
    inline Renderer& GetRenderer()
    {
        return *m_Renderer;
    }

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

    friend int ::main(int argc, char** argv);
};

} // namespace Krafter
