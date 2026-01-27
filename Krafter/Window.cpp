#include "glad/gl.h"

#include "GLFW/glfw3.h"

#include "Krafter/Renderer/Renderer.h"
#include "Krafter/Window.h"

namespace Krafter {

Window::Window()
    : m_Size(1280, 720)
{
    assert(!s_Instance);
    s_Instance = this;

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    m_Id = glfwCreateWindow(m_Size.x, m_Size.y, "Krafter", nullptr, nullptr);
    glfwMakeContextCurrent(m_Id);

    glfwSetFramebufferSizeCallback(m_Id, FramebufferSizeCallback);

    SetCursor(false);
}

Window::~Window()
{
    glfwDestroyWindow(m_Id);
    glfwTerminate();
}

bool Window::IsOpen() const
{
    return !glfwWindowShouldClose(m_Id);
}

void Window::Close() const
{
    glfwSetWindowShouldClose(m_Id, GLFW_TRUE);
}

void Window::PollEvents() const
{
    glfwPollEvents();
}

void Window::SwapBuffers() const
{
    glfwSwapBuffers(m_Id);
}

float Window::GetTime() const
{
    return glfwGetTime();
}

bool Window::IsKeyDown(Key key) const
{
    return glfwGetKey(m_Id, (int)key) == GLFW_PRESS;
}

void Window::SetCursor(bool enabled)
{
    glfwSetInputMode(m_Id, GLFW_CURSOR, enabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

glm::vec2 Window::GetCursorPosition() const
{
    double x;
    double y;
    glfwGetCursorPos(m_Id, &x, &y);
    return glm::vec2(x, y);
}

void Window::FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    s_Instance->m_Size.x = width;
    s_Instance->m_Size.y = height;

    glViewport(0, 0, s_Instance->GetSize().x, s_Instance->GetSize().y);

    Renderer::Get()->GetCamera().UpdateProjection();
}

} // namespace Krafter
