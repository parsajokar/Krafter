#include <string_view>
#include <utility>

#include "Krafter/Core/Application.h"
#include "Krafter/Core/Event.h"
#include "Krafter/Core/Window.h"
#include "Krafter/PauseMenuLayer.h"

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

// The hotbar slot art reused as the button face and its hover outline: a 22x22
// box at (15, 0) from the texture's bottom-left, the outline at (37, 0).
constexpr glm::vec2 k_SlotSprite = glm::vec2(22.0f, 22.0f);
constexpr float k_SlotSpriteX = 15.0f;
constexpr float k_OutlineSpriteX = 37.0f;

PauseMenuLayer::PauseMenuLayer(Window& window, std::function<void()> onResume, std::function<void()> onExitToMenu)
    : Layer("PauseMenu")
    , m_Window(window)
    , m_OnResume(std::move(onResume))
    , m_OnExitToMenu(std::move(onExitToMenu))
    , m_Renderer(window)
    , m_UITexture("assets/textures/ui.png")
    , m_Font("assets/textures/font.png")
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

bool PauseMenuLayer::Contains(const glm::vec4& rect, const glm::vec2& point)
{
    return point.x >= rect.x && point.x <= rect.x + rect.z
        && point.y >= rect.y && point.y <= rect.y + rect.w;
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

    DrawButton(resumeRect, k_ResumeLabel);
    DrawButton(ExitRect(), k_ExitLabel);

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
            if (Contains(ResumeRect(), m_Cursor)) {
                m_OnResume();
            } else if (Contains(ExitRect(), m_Cursor)) {
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

void PauseMenuLayer::DrawButton(const glm::vec4& rect, std::string_view label)
{
    const glm::vec2 texSize = glm::vec2(m_UITexture.GetSize());
    const glm::vec2 slotPos = glm::vec2(k_SlotSpriteX, texSize.y - k_SlotSprite.y);
    const glm::vec2 outlinePos = glm::vec2(k_OutlineSpriteX, texSize.y - k_SlotSprite.y);

    const glm::vec2 position = glm::vec2(rect);
    const glm::vec2 size = glm::vec2(rect.z, rect.w);
    const bool hovered = Contains(rect, m_Cursor);

    DrawSlicedSprite(slotPos, k_SlotSprite, position, size, hovered ? 0.85f : 0.6f);
    if (hovered) {
        DrawSlicedSprite(outlinePos, k_SlotSprite, position, size);
    }

    const glm::vec2 labelSize = glm::vec2(
        m_Font.Measure(label, k_ButtonTextScale), m_Font.LineHeight(k_ButtonTextScale));
    const glm::vec2 labelPos = glm::floor(position + (size - labelSize) * 0.5f);
    m_Font.Draw(m_Renderer, label, labelPos, k_ButtonTextScale, glm::vec4(1.0f));
}

void PauseMenuLayer::DrawSprite(
    const glm::vec2& spritePos, const glm::vec2& spriteSize,
    const glm::vec2& position, const glm::vec2& size, float opacity)
{
    const glm::vec2 texSize = glm::vec2(m_UITexture.GetSize());
    const glm::vec2 uvMin = spritePos / texSize;
    const glm::vec2 uvMax = (spritePos + spriteSize) / texSize;
    // Textures are flipped vertically on load, so flip V to keep the sprite upright.
    const glm::vec4 uvRect = glm::vec4(
        uvMin.x, 1.0f - uvMin.y, uvMax.x - uvMin.x, uvMin.y - uvMax.y);
    m_Renderer.DrawQuad(position, size, m_UITexture, uvRect, glm::vec4(1.0f, 1.0f, 1.0f, opacity));
}

void PauseMenuLayer::DrawSlicedSprite(
    const glm::vec2& spritePos, const glm::vec2& spriteSize,
    const glm::vec2& position, const glm::vec2& size, float opacity)
{
    const float srcThird = spriteSize.x / 3.0f;
    // The caps keep the sprite's aspect at the destination height; floor so the
    // three pieces meet on integer pixels and don't leave a seam.
    const float capWidth = glm::floor(srcThird * size.y / spriteSize.y);
    const float midWidth = size.x - 2.0f * capWidth;

    // Left cap.
    DrawSprite(
        spritePos, glm::vec2(srcThird, spriteSize.y),
        position, glm::vec2(capWidth, size.y), opacity);

    // Middle third, stretched across the gap between the caps.
    DrawSprite(
        spritePos + glm::vec2(srcThird, 0.0f), glm::vec2(srcThird, spriteSize.y),
        position + glm::vec2(capWidth, 0.0f), glm::vec2(midWidth, size.y), opacity);

    // Right cap: the remaining source width covers any rounding in the thirds.
    DrawSprite(
        spritePos + glm::vec2(2.0f * srcThird, 0.0f), glm::vec2(spriteSize.x - 2.0f * srcThird, spriteSize.y),
        position + glm::vec2(size.x - capWidth, 0.0f), glm::vec2(capWidth, size.y), opacity);
}

} // namespace Krafter
