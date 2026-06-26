#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <string_view>
#include <utility>

#include "imgui.h"

#include "Krafter/Core/Event.h"
#include "Krafter/Core/Window.h"
#include "Krafter/MainMenuLayer.h"

namespace Krafter {

constexpr glm::vec2 k_FieldSize = glm::vec2(280.0f, 50.0f);
constexpr glm::vec2 k_ButtonSize = glm::vec2(280.0f, 50.0f);
constexpr float k_Gap = 16.0f; // vertical space between stacked elements

constexpr float k_ButtonTextScale = 1.0f;
constexpr float k_FieldTextScale = 1.0f;
constexpr float k_FieldPadding = 14.0f;
constexpr int k_MaxSeedLength = 32;

constexpr std::string_view k_Title = "Krafter";
constexpr float k_TitleScale = 3.0f;
// Nudges the title off its centred spot: right and up, to taste.
constexpr glm::vec2 k_TitleOffset = glm::vec2(8.0f, -40.0f);

// The hotbar slot art: a 22x22 box at (15, 0) from the texture's bottom-left,
// with the selection outline at (37, 0). Reused here as the field/button and a
// hover/focus highlight.
constexpr glm::vec2 k_SlotSprite = glm::vec2(22.0f, 22.0f);
constexpr float k_SlotSpriteX = 15.0f;
constexpr float k_OutlineSpriteX = 37.0f;

MainMenuLayer::MainMenuLayer(Window& window, std::function<void(int32_t)> onPlay)
    : Layer("MainMenu")
    , m_Window(window)
    , m_OnPlay(std::move(onPlay))
    , m_Renderer(window)
    , m_UITexture("assets/textures/ui.png")
    , m_Background("assets/textures/main_menu.png")
    , m_Font("assets/textures/font.png")
{
}

void MainMenuLayer::OnAttach()
{
    // The window starts with the cursor captured for mouse-look; free it so the
    // menu can be clicked. Pressing "Play!" recaptures it for the player.
    m_Window.SetCursor(true);

    // A prior play session may have hidden the mouse from ImGui (the player does
    // this while controlled). Restore it so the menu and debug overlay respond.
    ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
}

glm::vec4 MainMenuLayer::SeedRect() const
{
    const glm::vec2 windowSize = glm::vec2(m_Window.GetSize());
    // The title stacks above the field, then the Play and Exit buttons, each
    // separated by a gap; the whole group is centred in the window.
    const float titleHeight = m_Font.LineHeight(k_TitleScale);
    const float groupHeight = titleHeight + k_Gap
        + k_FieldSize.y + k_Gap + k_ButtonSize.y + k_Gap + k_ButtonSize.y;
    // Snap to the pixel grid so the stretched sprites and text stay crisp.
    const float top = glm::floor((windowSize.y - groupHeight) * 0.5f);
    const float fieldTop = top + titleHeight + k_Gap;
    const float x = glm::floor((windowSize.x - k_FieldSize.x) * 0.5f);
    return glm::vec4(x, fieldTop, k_FieldSize);
}

glm::vec4 MainMenuLayer::ButtonRect() const
{
    const glm::vec4 seed = SeedRect();
    const glm::vec2 windowSize = glm::vec2(m_Window.GetSize());
    const float x = glm::floor((windowSize.x - k_ButtonSize.x) * 0.5f);
    return glm::vec4(x, seed.y + k_FieldSize.y + k_Gap, k_ButtonSize);
}

glm::vec4 MainMenuLayer::ExitRect() const
{
    const glm::vec4 play = ButtonRect();
    return glm::vec4(play.x, play.y + k_ButtonSize.y + k_Gap, k_ButtonSize);
}

bool MainMenuLayer::Contains(const glm::vec4& rect, const glm::vec2& point)
{
    return point.x >= rect.x && point.x <= rect.x + rect.z
        && point.y >= rect.y && point.y <= rect.y + rect.w;
}

void MainMenuLayer::OnRender()
{
    if (!m_Active) {
        return;
    }

    m_Renderer.Begin();

    // Backdrop: the menu background image stretched over the whole screen. The UV
    // rect flips V because textures load vertically flipped, keeping it upright.
    m_Renderer.DrawQuad(
        glm::vec2(0.0f), glm::vec2(m_Window.GetSize()), m_Background,
        glm::vec4(0.0f, 1.0f, 1.0f, -1.0f));

    const glm::vec2 texSize = glm::vec2(m_UITexture.GetSize());
    const glm::vec2 slotPos = glm::vec2(k_SlotSpriteX, texSize.y - k_SlotSprite.y);
    const glm::vec2 outlinePos = glm::vec2(k_OutlineSpriteX, texSize.y - k_SlotSprite.y);

    // Seed field.
    const glm::vec4 seedRect = SeedRect();
    const glm::vec2 seedPos = glm::vec2(seedRect);

    // Title, centred horizontally and sitting just above the field as the top of
    // the centred menu group.
    const float titleHeight = m_Font.LineHeight(k_TitleScale);
    const float titleWidth = m_Font.Measure(k_Title, k_TitleScale);
    const glm::vec2 titlePos = glm::floor(glm::vec2(
                                              (m_Window.GetSize().x - titleWidth) * 0.5f, seedPos.y - k_Gap - titleHeight)
        + k_TitleOffset);
    m_Font.Draw(m_Renderer, k_Title, titlePos, k_TitleScale, glm::vec4(1.0f));
    DrawSlicedSprite(slotPos, k_SlotSprite, seedPos, k_FieldSize, m_SeedFocused ? 0.85f : 0.6f);
    if (m_SeedFocused) {
        DrawSlicedSprite(outlinePos, k_SlotSprite, seedPos, k_FieldSize);
    }

    const float fieldTextHeight = m_Font.LineHeight(k_FieldTextScale);
    const float fieldTextTop = glm::floor(seedPos.y + (k_FieldSize.y - fieldTextHeight) * 0.5f);
    const float fieldTextLeft = seedPos.x + k_FieldPadding;

    if (m_Seed.empty() && !m_SeedFocused) {
        // Dim placeholder hinting at what the field is for.
        m_Font.Draw(m_Renderer, "Seed", glm::vec2(fieldTextLeft, fieldTextTop),
            k_FieldTextScale, glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
    } else {
        // Scroll so the text's end (where the caret sits) stays in view, and clip
        // to the field's interior so the overflow doesn't spill past the caps.
        const float innerWidth = k_FieldSize.x - 2.0f * k_FieldPadding;
        const float textWidth = m_Font.Measure(m_Seed, k_FieldTextScale);
        const float caretRoom = m_SeedFocused ? 3.0f : 0.0f;
        const float scrollX = glm::max(0.0f, textWidth + caretRoom - innerWidth);
        const float textX = glm::floor(fieldTextLeft - scrollX);

        m_Renderer.SetScissor(
            glm::vec2(fieldTextLeft, seedPos.y), glm::vec2(innerWidth, k_FieldSize.y));

        m_Font.Draw(m_Renderer, m_Seed, glm::vec2(textX, fieldTextTop), k_FieldTextScale, glm::vec4(1.0f));

        // Blinking caret at the end of the text, on for the first half of each second.
        if (m_SeedFocused && glm::fract(m_Window.GetTime()) < 0.5f) {
            m_Renderer.DrawQuad(glm::vec2(textX + textWidth + 1.0f, fieldTextTop),
                glm::vec2(2.0f, fieldTextHeight), glm::vec4(1.0f));
        }

        m_Renderer.ClearScissor();
    }

    DrawButton(ButtonRect(), "Play!");
    DrawButton(ExitRect(), "Exit");

    m_Renderer.End();
}

void MainMenuLayer::OnEvent(Event& event)
{
    if (!m_Active) {
        return;
    }

    switch (event.type) {
    case EventType::k_MouseMoved:
        m_Cursor = event.mouse;
        return;

    case EventType::k_MouseButtonPressed:
        if (event.button == MouseButton::k_Left) {
            // A click focuses the field, fires a button, or clears focus.
            if (Contains(SeedRect(), m_Cursor)) {
                m_SeedFocused = true;
            } else if (Contains(ButtonRect(), m_Cursor)) {
                Play();
            } else if (Contains(ExitRect(), m_Cursor)) {
                m_Window.Close();
            } else {
                m_SeedFocused = false;
            }
        }
        break;

    case EventType::k_TextInput:
        // Accept printable ASCII into the focused field, up to the length cap.
        if (m_SeedFocused && event.codepoint >= 32 && event.codepoint < 127
            && static_cast<int>(m_Seed.size()) < k_MaxSeedLength) {
            m_Seed.push_back(static_cast<char>(event.codepoint));
        }
        break;

    case EventType::k_KeyPressed:
        if (event.key == Key::k_Backspace && m_SeedFocused && !m_Seed.empty()) {
            m_Seed.pop_back();
        } else if (event.key == Key::k_Enter) {
            Play();
        }
        break;

    default:
        break;
    }

    // The menu owns every input while it is up, so nothing (block breaking, the
    // control toggle, hotbar keys) leaks to the world beneath it.
    event.handled = true;
}

int32_t MainMenuLayer::SeedFromText() const
{
    // Empty field: a fresh, time-based seed each launch.
    if (m_Seed.empty()) {
        return static_cast<int32_t>(std::time(nullptr));
    }

    // A plain integer (optionally signed) is used directly, like Minecraft.
    char* end = nullptr;
    const long long value = std::strtoll(m_Seed.c_str(), &end, 10);
    if (end != nullptr && *end == '\0') {
        return static_cast<int32_t>(value);
    }

    // Anything else is hashed (FNV-1a) so any text maps to a stable seed.
    uint32_t hash = 2166136261u;
    for (char ch : m_Seed) {
        hash ^= static_cast<uint8_t>(ch);
        hash *= 16777619u;
    }
    return static_cast<int32_t>(hash);
}

void MainMenuLayer::Play()
{
    m_Active = false;
    m_OnPlay(SeedFromText());
}

void MainMenuLayer::DrawButton(const glm::vec4& rect, std::string_view label)
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

void MainMenuLayer::DrawSprite(
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

void MainMenuLayer::DrawSlicedSprite(
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
