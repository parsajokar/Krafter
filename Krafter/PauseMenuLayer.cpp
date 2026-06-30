#include <string_view>
#include <utility>

#include "Krafter/Core/Application.h"
#include "Krafter/Core/Event.h"
#include "Krafter/Core/Window.h"
#include "Krafter/PauseMenuLayer.h"
#include "Krafter/Renderer/Font.h"
#include "Krafter/Renderer/UIRenderer.h"
#include "Krafter/Renderer/Widgets.h"

namespace Krafter {

constexpr float k_ButtonHeight = 50.0f;
constexpr float k_ButtonPadding = 24.0f; // horizontal space around the label
constexpr float k_MinButtonWidth = 280.0f;
constexpr float k_Gap = 16.0f; // vertical space between stacked buttons
constexpr float k_TitleGap = 48.0f; // extra space between the title and the buttons

constexpr float k_ButtonTextScale = 1.0f;

constexpr std::string_view k_Title = "Paused";
constexpr float k_TitleScale = 3.0f;

constexpr std::string_view k_ResumeLabel = "Resume";
constexpr std::string_view k_ExitLabel = "Exit to main menu";

// A translucent black veil over the frozen scene behind the menu.
constexpr glm::vec4 k_DimColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.6f);

PauseMenuLayer::PauseMenuLayer(
    Window& window, UIRenderer& renderer, Texture2D& uiTexture, Font& font,
    std::function<void()> onResume, std::function<void()> onExitToMenu)
    : UIScreen("PauseMenu", window, renderer, uiTexture, font)
    , m_OnResume(std::move(onResume))
    , m_OnExitToMenu(std::move(onExitToMenu))
{
}

float PauseMenuLayer::ButtonWidth() const
{
    const float widest = glm::max(
        m_Font.Measure(k_ResumeLabel, k_ButtonTextScale),
        m_Font.Measure(k_ExitLabel, k_ButtonTextScale));
    return glm::max(k_MinButtonWidth, widest + 2.0f * k_ButtonPadding);
}

glm::vec4 PauseMenuLayer::ResumeRect() const
{
    const glm::vec2 windowSize = glm::vec2(m_Window.GetSize());

    // The title sits a generous gap above two stacked buttons; the whole group is
    // centred in the window.
    const float buttonWidth = ButtonWidth();
    const float titleHeight = m_Font.LineHeight(k_TitleScale);
    const float groupHeight = titleHeight + k_TitleGap + 2.0f * k_ButtonHeight + k_Gap;

    // Snap to the pixel grid so the stretched sprites and text stay crisp.
    const float top = glm::floor((windowSize.y - groupHeight) * 0.5f);
    const float buttonTop = top + titleHeight + k_TitleGap;
    const float x = glm::floor((windowSize.x - buttonWidth) * 0.5f);
    return glm::vec4(x, buttonTop, buttonWidth, k_ButtonHeight);
}

glm::vec4 PauseMenuLayer::ExitRect() const
{
    const glm::vec4 resume = ResumeRect();
    return glm::vec4(resume.x, resume.y + k_ButtonHeight + k_Gap, resume.z, k_ButtonHeight);
}

void PauseMenuLayer::OnRender()
{
    m_Renderer.Begin();

    // Dim the world behind the menu.
    m_Renderer.DrawQuad(glm::vec2(0.0f), glm::vec2(m_Window.GetSize()), k_DimColor);

    const glm::vec4 resumeRect = ResumeRect();

    // Title, centred horizontally and sitting a generous gap above the buttons.
    const float titleHeight = m_Font.LineHeight(k_TitleScale);
    const float titleWidth = m_Font.Measure(k_Title, k_TitleScale);
    const glm::vec2 titlePos = glm::floor(glm::vec2(
        (m_Window.GetSize().x - titleWidth) * 0.5f, resumeRect.y - k_TitleGap - titleHeight));
    m_Font.Draw(m_Renderer, k_Title, titlePos, k_TitleScale, glm::vec4(1.0f));

    DrawMenuButton(m_Renderer, m_Font, m_UITexture, resumeRect, k_ResumeLabel, RectContains(resumeRect, m_Cursor));
    const glm::vec4 exitRect = ExitRect();
    DrawMenuButton(m_Renderer, m_Font, m_UITexture, exitRect, k_ExitLabel, RectContains(exitRect, m_Cursor));

    m_Renderer.End();
}

void PauseMenuLayer::OnEvent(Event& event)
{
    switch (event.type) {
    case EventType::k_MouseMoved:
        m_Cursor = event.mouse;
        break;

    case EventType::k_MouseButtonPressed:
        if (event.button == MouseButton::k_Left) {
            if (RectContains(ResumeRect(), m_Cursor)) {
                m_OnResume();
            } else if (RectContains(ExitRect(), m_Cursor)) {
                m_OnExitToMenu();
            }
        }
        break;

    case EventType::k_KeyPressed:
        // Escape resumes play, closing the pause menu.
        if (event.key == Key::k_Escape && !event.isRepeat) {
            m_OnResume();
        } else if (event.key == Key::k_F3 && !event.isRepeat) {
            // F3 keeps toggling the debug overlay while paused, as in Minecraft.
            Application::Get().ToggleDebugUI();
        }
        break;

    default:
        break;
    }

    // The pause menu owns every input while it is up, so nothing leaks to the
    // world beneath it.
    event.handled = true;
}

} // namespace Krafter
