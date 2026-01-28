#include "glad/gl.h"

#include "GLFW/glfw3.h"

#include "Krafter/Renderer/Renderer.h"
#include "Krafter/Window.h"

namespace Krafter {

void Window::Init()
{
    assert(!s_Window);
    s_Window = new Window();
}

void Window::Shutdown()
{
    delete s_Window;
}

bool Window::IsOpen()
{
    return !glfwWindowShouldClose(s_Window->m_Id);
}

void Window::Close()
{
    glfwSetWindowShouldClose(s_Window->m_Id, GLFW_TRUE);
}

void Window::PollEvents()
{
    glfwPollEvents();

    float currentFrameTime = GetTime();
    s_Window->m_Delta = currentFrameTime - s_Window->m_LastFrameTime;
    s_Window->m_LastFrameTime = currentFrameTime;
}

void Window::SwapBuffers()
{
    glfwSwapBuffers(s_Window->m_Id);
}

float Window::GetTime()
{
    return glfwGetTime();
}

bool Window::IsKeyDown(Key key)
{
    return glfwGetKey(s_Window->m_Id, (int)key) == GLFW_PRESS;
}

void Window::SetCursor(bool enabled)
{
    glfwSetInputMode(s_Window->m_Id, GLFW_CURSOR, enabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

glm::vec2 Window::GetCursorPosition()
{
    double x;
    double y;
    glfwGetCursorPos(s_Window->m_Id, &x, &y);
    return glm::vec2(x, y);
}

Window::Window()
    : m_Size(1280, 720)
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    m_Id = glfwCreateWindow(m_Size.x, m_Size.y, "Krafter", nullptr, nullptr);
    glfwMakeContextCurrent(m_Id);

    glfwSetFramebufferSizeCallback(m_Id, FramebufferSizeCallback);

    glfwSetInputMode(m_Id, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

Window::~Window()
{
    glfwDestroyWindow(m_Id);
    glfwTerminate();
}

void Window::FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    s_Window->m_Size.x = width;
    s_Window->m_Size.y = height;

    glViewport(0, 0, s_Window->GetSize().x, s_Window->GetSize().y);

    Renderer::GetCamera()->UpdateProjection();
}

} // namespace Krafter
