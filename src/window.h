#pragma once

#include "glm/glm.hpp"

typedef struct GLFWwindow GLFWwindow;
using WindowId = GLFWwindow*;

namespace Krafter {

enum class Key : int {
    ESCAPE = 256,
    SPACE = 32,
    W = 87,
    S = 83,
    D = 68,
    A = 65,
};

class Window {
public:
    inline static Window* Get()
    {
        return _instance;
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
        return _id;
    }
    inline const glm::ivec2& GetSize() const
    {
        return _size;
    }

private:
    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);

    inline static Window* _instance = nullptr;

    WindowId _id;
    glm::ivec2 _size;
};

} // namespace Krafter
