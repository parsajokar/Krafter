#pragma once

#include "glm/glm.hpp"

typedef struct GLFWwindow GLFWwindow;
using WindowId = GLFWwindow*;

namespace Krafter {

enum class Key : int {
    k_Escape = 256,
    k_Space = 32,
    k_W = 87,
    k_S = 83,
    k_D = 68,
    k_A = 65,
};

class Window {
public:
    static void Init(); // !!! MAKE SURE TO CALL SHUTDOWN
    static void Shutdown();

    static bool IsOpen();
    static void Close();

    static void PollEvents();
    static void SwapBuffers();

    static float GetTime();

    static bool IsKeyDown(Key key);

    static void SetCursor(bool enabled);
    static glm::vec2 GetCursorPosition();

    inline static WindowId GetId()
    {
        return s_Window->m_Id;
    }

    inline static const glm::ivec2& GetSize()
    {
        return s_Window->m_Size;
    }

    inline static float GetDelta()
    {
        return s_Window->m_Delta;
    }

private:
    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);

    inline static Window* s_Window = nullptr;

    Window();
    ~Window();

    WindowId m_Id;
    glm::ivec2 m_Size;

    float m_LastFrameTime = 0.0f;
    float m_Delta = 0.0f;
};

} // namespace Krafter
