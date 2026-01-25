#include "glad/gl.h"

#include "GLFW/glfw3.h"

#include "renderer/renderer.h"
#include "window.h"

namespace Krafter {

Window::Window()
    : _size(1280, 720)
{
    assert(!_instance);
    _instance = this;

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    _id = glfwCreateWindow(_size.x, _size.y, "Krafter", nullptr, nullptr);
    glfwMakeContextCurrent(_id);

    glfwSetFramebufferSizeCallback(_id, FramebufferSizeCallback);

    SetCursor(false);
}

Window::~Window()
{
    glfwDestroyWindow(_id);
    glfwTerminate();
}

bool Window::IsOpen() const
{
    return !glfwWindowShouldClose(_id);
}

void Window::Close() const
{
    glfwSetWindowShouldClose(_id, GLFW_TRUE);
}

void Window::PollEvents() const
{
    glfwPollEvents();
}

void Window::SwapBuffers() const
{
    glfwSwapBuffers(_id);
}

float Window::GetTime() const
{
    return glfwGetTime();
}

bool Window::IsKeyDown(Key key) const
{
    return glfwGetKey(_id, (int)key) == GLFW_PRESS;
}

void Window::SetCursor(bool enabled)
{
    glfwSetInputMode(_id, GLFW_CURSOR, enabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

glm::vec2 Window::GetCursorPosition() const
{
    double x;
    double y;
    glfwGetCursorPos(_id, &x, &y);
    return glm::vec2(x, y);
}

void Window::FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    _instance->_size.x = width;
    _instance->_size.y = height;

    glViewport(0, 0, _instance->GetSize().x, _instance->GetSize().y);

    Renderer::Get()->GetCamera().UpdateProjection();
}

} // namespace Krafter
