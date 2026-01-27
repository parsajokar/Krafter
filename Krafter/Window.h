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
    inline static Window* Get()
    {
        return s_Instance;
    }

    Window();
    ~Window();

    bool IsOpen() const;
    void Close() const;

    void PollEvents() const;
    void SwapBuffers() const;

    float GetTime() const;

    bool IsKeyDown(Key key) const;

    void SetCursor(bool enabled);
    glm::vec2 GetCursorPosition() const;

    inline WindowId GetId() const
    {
        return m_Id;
    }
    inline const glm::ivec2& GetSize() const
    {
        return m_Size;
    }

private:
    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);

    inline static Window* s_Instance = nullptr;

    WindowId m_Id;
    glm::ivec2 m_Size;
};

} // namespace Krafter
