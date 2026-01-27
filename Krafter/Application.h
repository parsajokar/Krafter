#pragma once

#include <memory>
#include <string>

#include "Krafter/ImGuiOverlay.h"
#include "Krafter/LayerStack.h"
#include "Krafter/Renderer/Renderer.h"
#include "Krafter/Window.h"

int main(int argc, char** argv);

namespace Krafter {

struct ApplicationSpec {
    std::string name;
    std::string workingDirectory;
};

class Application {
public:
    inline static Application* Get()
    {
        return s_Instance;
    }

    Application(const ApplicationSpec& spec);
    virtual ~Application();

    void PushLayer(Layer* layer);
    void PushOverlay(Layer* layer);

    inline float GetDelta() const
    {
        return m_Delta;
    }

private:
    inline static Application* s_Instance = nullptr;

    void Run();

    ApplicationSpec m_Spec;

    std::unique_ptr<Window> m_Window;

    Renderer* m_Renderer;
    ImGuiOverlay* m_ImGuiOverlay;
    LayerStack m_LayerStack;

    float m_LastFrameTime = 0.0f;
    float m_Delta = 0.0f;

    friend int ::main(int argc, char** argv);
};

} // namespace Krafter
