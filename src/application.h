#pragma once

#include <memory>
#include <string>

#include "imgui_overlay.h"
#include "layer_stack.h"
#include "renderer/renderer.h"
#include "window.h"

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
        return _instance;
    }

    Application(const ApplicationSpec& spec);
    virtual ~Application();

    void PushLayer(Layer* layer);
    void PushOverlay(Layer* layer);

    inline float GetDelta() const
    {
        return _delta;
    }

private:
    inline static Application* _instance = nullptr;

    void Run();

    ApplicationSpec _spec;

    std::unique_ptr<Window> _window;

    Renderer* _renderer;
    ImGuiOverlay* _imguiOverlay;
    LayerStack _layerStack;

    float _lastFrameTime = 0.0f;
    float _delta = 0.0f;

    friend int ::main(int argc, char** argv);
};

} // namespace Krafter
