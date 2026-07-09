#include "GLFW/glfw3.h"

#include "Krafter/Core/Event.h"
#include "Krafter/Core/Window.h"

namespace Krafter {

Window::Window()
    : m_Size(1280, 720)
{
    glfwInit();
    // No GL context: the Vulkan renderer creates a surface from this window and
    // owns all device state itself.
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_Id = glfwCreateWindow(m_Size.x, m_Size.y, "Krafter", nullptr, nullptr);

    glfwSetWindowUserPointer(m_Id, this);
    glfwSetFramebufferSizeCallback(m_Id, FramebufferSizeCallback);
    glfwSetKeyCallback(m_Id, KeyCallback);
    glfwSetCharCallback(m_Id, CharCallback);
    glfwSetMouseButtonCallback(m_Id, MouseButtonCallback);
    glfwSetCursorPosCallback(m_Id, CursorPositionCallback);
    glfwSetScrollCallback(m_Id, ScrollCallback);

    glfwSetInputMode(m_Id, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
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

void Window::Close()
{
    glfwSetWindowShouldClose(m_Id, GLFW_TRUE);
}

void Window::PollEvents()
{
    glfwPollEvents();

    float currentFrameTime = GetTime();
    m_Delta = currentFrameTime - m_LastFrameTime;
    m_LastFrameTime = currentFrameTime;

    // Refresh the averaged FPS a few times a second so the readout stays legible.
    m_FpsAccumTime += m_Delta;
    m_FpsFrameCount++;
    if (m_FpsAccumTime >= k_FpsUpdateInterval) {
        m_Fps = static_cast<float>(m_FpsFrameCount) / m_FpsAccumTime;
        m_FpsAccumTime = 0.0f;
        m_FpsFrameCount = 0;
    }
}

void Window::SetEventCallback(const EventCallback& callback)
{
    m_EventCallback = callback;
}

float Window::GetTime() const
{
    return glfwGetTime();
}

void Window::SetCursor(bool enabled)
{
    glfwSetInputMode(m_Id, GLFW_CURSOR, enabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

void Window::ToggleFullscreen()
{
    if (m_Fullscreen) {
        glfwSetWindowMonitor(
            m_Id, nullptr,
            m_WindowedPos.x, m_WindowedPos.y, m_WindowedSize.x, m_WindowedSize.y, 0);
        m_Fullscreen = false;
        return;
    }

    // Remember the windowed placement so it can be restored later.
    glfwGetWindowPos(m_Id, &m_WindowedPos.x, &m_WindowedPos.y);
    glfwGetWindowSize(m_Id, &m_WindowedSize.x, &m_WindowedSize.y);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    glfwSetWindowMonitor(m_Id, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    m_Fullscreen = true;
}

void Window::FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

    self->m_Size.x = width;
    self->m_Size.y = height;

    // The Vulkan renderer recreates its swapchain from the framebuffer size on
    // the next frame; there is no GL viewport to update here.

    if (!self->m_EventCallback) {
        return;
    }

    Event event;
    event.type = EventType::k_WindowResized;
    event.size = self->m_Size;
    self->m_EventCallback(event);
}

void Window::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (!self->m_EventCallback) {
        return;
    }

    Event event;
    event.type = action == GLFW_RELEASE ? EventType::k_KeyReleased : EventType::k_KeyPressed;
    event.key = (Key)key;
    event.isRepeat = action == GLFW_REPEAT;
    self->m_EventCallback(event);
}

void Window::CharCallback(GLFWwindow* window, unsigned int codepoint)
{
    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (!self->m_EventCallback) {
        return;
    }

    Event event;
    event.type = EventType::k_TextInput;
    event.codepoint = codepoint;
    self->m_EventCallback(event);
}

void Window::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (!self->m_EventCallback) {
        return;
    }

    Event event;
    event.type = action == GLFW_RELEASE ? EventType::k_MouseButtonReleased : EventType::k_MouseButtonPressed;
    event.button = (MouseButton)button;
    self->m_EventCallback(event);
}

void Window::CursorPositionCallback(GLFWwindow* window, double x, double y)
{
    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (!self->m_EventCallback) {
        return;
    }

    Event event;
    event.type = EventType::k_MouseMoved;
    event.mouse = glm::vec2(x, y);
    self->m_EventCallback(event);
}

void Window::ScrollCallback(GLFWwindow* window, double x, double y)
{
    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (!self->m_EventCallback) {
        return;
    }

    Event event;
    event.type = EventType::k_MouseScrolled;
    event.mouse = glm::vec2(x, y);
    self->m_EventCallback(event);
}

} // namespace Krafter
