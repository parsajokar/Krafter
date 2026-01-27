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
        return s_instance;
    }

    Application(const ApplicationSpec& spec);
    virtual ~Application();

    void PushLayer(Layer* layer);
    void PushOverlay(Layer* layer);

    inline float GetDelta() const
    {
        return m_delta;
    }

private:
    inline static Application* s_instance = nullptr;

    void Run();

    ApplicationSpec m_spec;

    std::unique_ptr<Window> m_window;

    Renderer* m_renderer;
    ImGuiOverlay* m_imguiOverlay;
    LayerStack m_layerStack;

    float m_lastFrameTime = 0.0f;
    float m_delta = 0.0f;

    friend int ::main(int argc, char** argv);
};

} // namespace Krafter
