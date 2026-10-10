#pragma once

#include <functional>

#include "glm/glm.hpp"

typedef struct GLFWwindow GLFWwindow;
using WindowId = GLFWwindow*;

namespace Krafter {

struct Event;
using EventCallback = std::function<void(Event&)>;

enum class Key : int {
    k_Escape = 256,
    k_Enter = 257,
    k_Backspace = 259,
    k_F3 = 292,
    k_F11 = 300,
    k_Space = 32,
    k_W = 87,
    k_S = 83,
    k_D = 68,
    k_A = 65,
    k_E = 69,
    k_0 = 48,
    k_1 = 49,
    k_2 = 50,
    k_3 = 51,
    k_4 = 52,
    k_5 = 53,
    k_6 = 54,
    k_7 = 55,
    k_8 = 56,
    k_9 = 57,
};

enum class MouseButton : int {
    k_Left = 0,
    k_Right = 1,
    k_Middle = 2,
};

class Window {
public:
    Window();
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool IsOpen() const;
    void Close();

    void PollEvents();

    void SetEventCallback(const EventCallback& callback);

    float GetTime() const;

    void SetCursor(bool enabled);

    void ToggleFullscreen();

    inline WindowId GetId() const
    {
        return m_Id;
    }

    inline const glm::ivec2& GetSize() const
    {
        return m_Size;
    }

    inline float GetDelta() const
    {
        return m_Delta;
    }

    inline float GetFps() const
    {
        return m_Fps;
    }

private:
    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void CharCallback(GLFWwindow* window, unsigned int codepoint);
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void CursorPositionCallback(GLFWwindow* window, double x, double y);
    static void ScrollCallback(GLFWwindow* window, double x, double y);

    WindowId m_Id;
    glm::ivec2 m_Size;

    EventCallback m_EventCallback;

    bool m_Fullscreen = false;
    glm::ivec2 m_WindowedPos = glm::ivec2(0);
    glm::ivec2 m_WindowedSize = glm::ivec2(0);

    float m_LastFrameTime = 0.0f;
    float m_Delta = 0.0f;

    static constexpr float k_FpsUpdateInterval = 0.2f;
    float m_Fps = 0.0f;
    float m_FpsAccumTime = 0.0f;
    int32_t m_FpsFrameCount = 0;
};

}
