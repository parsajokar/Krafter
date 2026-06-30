#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <string_view>
#include <utility>

#include "imgui.h"

#include "Krafter/Core/Event.h"
#include "Krafter/Core/Window.h"
#include "Krafter/MainMenuLayer.h"
#include "Krafter/Renderer/Font.h"
#include "Krafter/Renderer/UIRenderer.h"
#include "Krafter/Renderer/Widgets.h"

namespace Krafter {

constexpr glm::vec2 k_FieldSize = glm::vec2(280.0f, 50.0f);
constexpr glm::vec2 k_ButtonSize = glm::vec2(280.0f, 50.0f);
constexpr float k_Gap = 16.0f; // vertical space between stacked elements

constexpr float k_FieldTextScale = 1.0f;
constexpr float k_FieldPadding = 14.0f;
constexpr int k_MaxSeedLength = 32;

constexpr std::string_view k_Title = "Krafter";
constexpr float k_TitleScale = 3.0f;
// Nudges the title off its centred spot: right and up, to taste.
constexpr glm::vec2 k_TitleOffset = glm::vec2(8.0f, -40.0f);

MainMenuLayer::MainMenuLayer(
    Window& window, UIRenderer& renderer, Texture2D& uiTexture, Font& font,
    std::function<void(int32_t, GameMode)> onPlay)
    : UIScreen("MainMenu", window, renderer, uiTexture, font)
    , m_OnPlay(std::move(onPlay))
    , m_Background("assets/textures/main_menu.png")
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
    // Title, seed field, then two button rows: Survive and Spectate share one row,
    // with Exit on its own row below. Each element is separated by a gap.
    const float groupHeight = titleHeight + k_Gap + k_FieldSize.y
        + 2.0f * (k_Gap + k_ButtonSize.y);
    // Snap to the pixel grid so the stretched sprites and text stay crisp.
    const float top = glm::floor((windowSize.y - groupHeight) * 0.5f);
    const float fieldTop = top + titleHeight + k_Gap;
    const float x = glm::floor((windowSize.x - k_FieldSize.x) * 0.5f);
    return glm::vec4(x, fieldTop, k_FieldSize);
}

glm::vec4 MainMenuLayer::SurviveRect() const
{
    // Left half of the button row below the seed field. The two halves split the
    // field's width with a gap between them, so the row lines up with the field.
    const glm::vec4 seed = SeedRect();
    const float halfWidth = glm::floor((k_ButtonSize.x - k_Gap) * 0.5f);
    return glm::vec4(seed.x, seed.y + k_FieldSize.y + k_Gap, halfWidth, k_ButtonSize.y);
}

glm::vec4 MainMenuLayer::SpectateRect() const
{
    // Right half of the same row, aligned to the field's right edge.
    const glm::vec4 survive = SurviveRect();
    const float x = survive.x + k_ButtonSize.x - survive.z;
    return glm::vec4(x, survive.y, survive.z, survive.w);
}

glm::vec4 MainMenuLayer::ExitRect() const
{
    // Full-width row beneath the Survive/Spectate row.
    const glm::vec4 survive = SurviveRect();
    return glm::vec4(survive.x, survive.y + k_ButtonSize.y + k_Gap, k_ButtonSize);
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
    DrawSlotSliced(m_Renderer, m_UITexture, seedPos, k_FieldSize,
        glm::vec4(1.0f, 1.0f, 1.0f, m_SeedFocused ? 0.85f : 0.6f));
    if (m_SeedFocused) {
        DrawSlotOutlineSliced(m_Renderer, m_UITexture, seedPos, k_FieldSize);
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

    const glm::vec4 surviveRect = SurviveRect();
    const glm::vec4 spectateRect = SpectateRect();
    const glm::vec4 exitRect = ExitRect();
    DrawMenuButton(m_Renderer, m_Font, m_UITexture, surviveRect, "Survive", RectContains(surviveRect, m_Cursor));
    DrawMenuButton(m_Renderer, m_Font, m_UITexture, spectateRect, "Spectate", RectContains(spectateRect, m_Cursor));
    DrawMenuButton(m_Renderer, m_Font, m_UITexture, exitRect, "Exit", RectContains(exitRect, m_Cursor));

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
            if (RectContains(SeedRect(), m_Cursor)) {
                m_SeedFocused = true;
            } else if (RectContains(SurviveRect(), m_Cursor)) {
                Play(GameMode::k_Survival);
            } else if (RectContains(SpectateRect(), m_Cursor)) {
                Play(GameMode::k_Spectator);
            } else if (RectContains(ExitRect(), m_Cursor)) {
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
            // Enter is the quick path into the default mode, Survive.
            Play(GameMode::k_Survival);
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

void MainMenuLayer::Play(GameMode mode)
{
    m_Active = false;
    m_OnPlay(SeedFromText(), mode);
}

} // namespace Krafter
