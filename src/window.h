#pragma once

#include "glm/glm.hpp"

typedef struct GLFWwindow GLFWwindow;
using WindowId = GLFWwindow*;

namespace Krafter
{

enum class Key : int
{
    ESCAPE = 256,
    SPACE = 32,
    W = 87,
    S = 83,
    D = 68,
    A = 65,
};

class Window
{
public:
    static void Init();
    static void Deinit();
    inline static Window* Get()
    {
        return _instance;
    }

    bool IsOpen() const;
    void Close() const;

    void PollEvents() const;
    void SwapBuffers() const;

    float GetTime() const;

    bool IsKeyDown(Key key) const;

    void EnableCursor(bool state) const;
    glm::vec2 GetCursorPosition() const;

    inline WindowId GetId() const
    {
        return _id;
    }
    inline const glm::uvec2& GetSize() const
    {
        return _size;
    }

private:
    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);

    inline static Window* _instance;

    Window();
    ~Window();

    WindowId _id;
    glm::uvec2 _size;
};

} // namespace Krafter
