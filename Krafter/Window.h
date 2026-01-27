#pragma once

#include "glm/glm.hpp"

typedef struct GLFWwindow GLFWwindow;
using WindowId = GLFWwindow*;

namespace Krafter {

enum class Key : int {
    Escape = 256,
    Space = 32,
    W = 87,
    S = 83,
    D = 68,
    A = 65,
};

class Window {
public:
    inline static Window* Get()
    {
        return s_instance;
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
        return m_id;
    }
    inline const glm::ivec2& GetSize() const
    {
        return m_size;
    }

private:
    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);

    inline static Window* s_instance = nullptr;

    WindowId m_id;
    glm::ivec2 m_size;
};

} // namespace Krafter
