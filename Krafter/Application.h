#pragma once

#include <memory>
#include <string>

#include "Krafter/LayerStack.h"
#include "Krafter/Renderer/Renderer.h"
#include "Krafter/Window.h"

int main(int argc, char** argv);

namespace Krafter {

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

private:
    inline static Application* s_Application = nullptr;

    void Run();

    void InitImGui();
    void ShutdownImGui();

    void BeginImGui();
    void EndImGui();

    ApplicationSpecification m_Specification;
    LayerStack m_LayerStack;

    friend int ::main(int argc, char** argv);
};

} // namespace Krafter
