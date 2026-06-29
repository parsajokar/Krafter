#include <algorithm>
#include <string_view>
#include <utility>

#include "Krafter/Core/Application.h"
#include "Krafter/Core/Event.h"
#include "Krafter/Core/Window.h"
#include "Krafter/Hotbar.h"
#include "Krafter/Inventory.h"
#include "Krafter/InventoryLayer.h"
#include "Krafter/Renderer/Widgets.h"

namespace Krafter {

namespace {

    // The slot art reused from the hotbar: a 22x22 box at (15, 0) from the texture's
    // bottom-left, drawn at 2x like the rest of the HUD. The selection outline that
    // marks the hovered slot is the same 22x22 box at (37, 0).
    constexpr glm::vec2 k_SpriteSize = glm::vec2(22.0f, 22.0f);
    constexpr float k_SlotSpriteX = 15.0f;
    constexpr float k_OutlineSpriteX = 37.0f;
    constexpr float k_Scale = 2.0f;
    constexpr float k_Spacing = 2.0f;

    // The gap between the inventory grid and the hotbar row below it, separating the
    // two the way Minecraft's inventory does.
    constexpr float k_HotbarGap = 12.0f;

    // Translucent slots and a black veil over the frozen scene, matching the HUD's
    // opacity and the pause menu's dim.
    constexpr float k_SlotOpacity = 0.6f;
    constexpr glm::vec4 k_DimColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.6f);

    constexpr std::string_view k_Title = "Inventory";
    constexpr float k_TitleScale = 2.0f;
    constexpr float k_TitleGap = 16.0f; // space between the title and the grid

    // The slot box and the stride from one slot to the next, in pixels.
    constexpr float k_SlotSize = k_SpriteSize.x * k_Scale;
    constexpr float k_Stride = k_SlotSize + k_Spacing * k_Scale;

    // The icon inset within a slot, matching the hotbar so the icons line up. The
    // inset box, the icon size, and its offset are all square, so kept as scalars.
    constexpr float k_IconInset = 4.0f * k_Scale;
    constexpr float k_IconScale = 0.87f;
    constexpr float k_InsetSize = k_SlotSize - 2.0f * k_IconInset;
    constexpr float k_IconSize = k_InsetSize * k_IconScale;
    constexpr float k_IconOffset = k_IconInset + (k_InsetSize - k_IconSize) * 0.5f;

    // The panel spans 10 columns; the inventory grid plus the hotbar row below it.
    constexpr float k_PanelWidth = k_Stride * Inventory::k_Columns - k_Spacing * k_Scale;
    constexpr float k_GridHeight = k_Stride * Inventory::k_Rows - k_Spacing * k_Scale;
    constexpr float k_PanelHeight = k_GridHeight + k_HotbarGap + k_SlotSize;

    // Slot indices [0, k_GridSlots) address the inventory grid; the hotbar row
    // follows at [k_GridSlots, k_TotalSlots).
    constexpr int k_GridSlots = Inventory::k_SlotCount;
    constexpr int k_TotalSlots = k_GridSlots + Hotbar::k_SlotCount;

} // namespace

InventoryLayer::InventoryLayer(
    Window& window, UIRenderer& renderer, Texture2D& uiTexture, Font& font,
    Inventory& inventory, Hotbar& hotbar, std::function<void()> onClose)
    : Layer("Inventory")
    , m_Window(window)
    , m_OnClose(std::move(onClose))
    , m_Renderer(renderer)
    , m_UITexture(uiTexture)
    , m_Font(font)
    , m_Inventory(inventory)
    , m_Hotbar(hotbar)
    , m_BlockTexture("assets/textures/blocks.png")
    , m_ItemTexture("assets/textures/items.png")
{
}

glm::vec2 InventoryLayer::PanelOrigin() const
{
    const glm::vec2 windowSize = glm::vec2(m_Window.GetSize());
    // Snap to the pixel grid so the stretched sprites and icons stay crisp.
    return glm::floor((windowSize - glm::vec2(k_PanelWidth, k_PanelHeight)) * 0.5f);
}

glm::vec4 InventoryLayer::SlotRect(int index) const
{
    const glm::vec2 origin = PanelOrigin();
    if (index < k_GridSlots) {
        const int row = index / Inventory::k_Columns;
        const int col = index % Inventory::k_Columns;
        const glm::vec2 position = origin + glm::vec2(col * k_Stride, row * k_Stride);
        return glm::vec4(position, k_SlotSize, k_SlotSize);
    }

    // The hotbar row sits beneath the grid, across the gap.
    const int col = index - k_GridSlots;
    const glm::vec2 position = glm::vec2(
        origin.x + col * k_Stride, origin.y + k_GridHeight + k_HotbarGap);
    return glm::vec4(position, k_SlotSize, k_SlotSize);
}

int InventoryLayer::SlotAt(const glm::vec2& point) const
{
    for (int i = 0; i < k_TotalSlots; ++i) {
        if (RectContains(SlotRect(i), point)) {
            return i;
        }
    }
    return -1;
}

Item InventoryLayer::GetSlot(int index) const
{
    if (index < k_GridSlots) {
        return m_Inventory.GetItem(index);
    }
    return m_Hotbar.GetItem(index - k_GridSlots);
}

void InventoryLayer::SetSlot(int index, Item item)
{
    if (index < k_GridSlots) {
        m_Inventory.SetItem(index, item);
    } else {
        m_Hotbar.SetItem(index - k_GridSlots, item);
    }
}

void InventoryLayer::ClickSlot(int index)
{
    const Item slotItem = GetSlot(index);

    // Same block in hand and in the slot: pour the held stack in, up to the
    // slot's k_MaxStack, and keep any overflow on the cursor.
    if (m_Held.IsBlock() && slotItem.IsBlock() && m_Held.block == slotItem.block
        && slotItem.count < Item::k_MaxStack) {
        const int moved = std::min(Item::k_MaxStack - slotItem.count, m_Held.count);
        Item merged = slotItem;
        merged.count += moved;
        SetSlot(index, merged);
        m_Held.count -= moved;
        if (m_Held.count <= 0) {
            m_Held = Item();
        }
        return;
    }

    // Otherwise swap the clicked slot with what the cursor is holding: pick up an
    // item from a filled slot, drop into an empty one, or swap two different ones.
    SetSlot(index, m_Held);
    m_Held = slotItem;
}

void InventoryLayer::Close()
{
    // Return any held item to the first free slot so it isn't lost when this
    // layer (and its held state) is destroyed. A free slot always exists: the one
    // it was picked up from was left empty.
    if (!m_Held.IsEmpty()) {
        for (int i = 0; i < k_TotalSlots; ++i) {
            if (GetSlot(i).IsEmpty()) {
                SetSlot(i, m_Held);
                m_Held = Item();
                break;
            }
        }
    }

    m_OnClose();
}

void InventoryLayer::OnRender()
{
    m_Renderer.Begin();

    // Dim the world and HUD behind the screen.
    m_Renderer.DrawQuad(glm::vec2(0.0f), glm::vec2(m_Window.GetSize()), k_DimColor);

    const glm::vec2 origin = PanelOrigin();

    // Title, centred over the panel and sitting a gap above the grid.
    const float titleWidth = m_Font.Measure(k_Title, k_TitleScale);
    const glm::vec2 titlePos = glm::floor(glm::vec2(
        origin.x + (k_PanelWidth - titleWidth) * 0.5f,
        origin.y - k_TitleGap - m_Font.LineHeight(k_TitleScale)));
    m_Font.Draw(m_Renderer, k_Title, titlePos, k_TitleScale, glm::vec4(1.0f));

    const glm::vec2 texSize = glm::vec2(m_UITexture.GetSize());
    const glm::vec2 slotSprite = glm::vec2(k_SlotSpriteX, texSize.y - k_SpriteSize.y);
    const glm::vec2 outlineSprite = glm::vec2(k_OutlineSpriteX, texSize.y - k_SpriteSize.y);

    // The slot under the cursor, highlighted with the selection outline.
    const int hovered = SlotAt(m_Cursor);

    // Every slot: the box sprite, with its block icon centred inside when filled,
    // and the outline on top when the cursor is over it.
    for (int i = 0; i < k_TotalSlots; ++i) {
        const glm::vec2 position = glm::vec2(SlotRect(i));
        m_Renderer.DrawSprite(m_UITexture, slotSprite, k_SpriteSize, position,
            glm::vec2(k_SlotSize), glm::vec4(1.0f, 1.0f, 1.0f, k_SlotOpacity));

        const Item item = GetSlot(i);
        if (!item.IsEmpty()) {
            DrawItemIcon(m_Renderer, m_BlockTexture, m_ItemTexture, item,
                position + glm::vec2(k_IconOffset), glm::vec2(k_IconSize));
            DrawItemCount(m_Renderer, m_Font, item.count, position, glm::vec2(k_SlotSize));
        }

        if (i == hovered) {
            m_Renderer.DrawSprite(m_UITexture, outlineSprite, k_SpriteSize, position,
                glm::vec2(k_SlotSize), glm::vec4(1.0f));
        }
    }

    // The held item trails the cursor, drawn last so it sits above the slots.
    if (!m_Held.IsEmpty()) {
        const glm::vec2 heldPos = m_Cursor - glm::vec2(k_IconSize * 0.5f);
        DrawItemIcon(m_Renderer, m_BlockTexture, m_ItemTexture, m_Held,
            heldPos, glm::vec2(k_IconSize));
        DrawItemCount(m_Renderer, m_Font, m_Held.count, heldPos, glm::vec2(k_IconSize));
    }

    m_Renderer.End();
}

void InventoryLayer::OnEvent(Event& event)
{
    switch (event.type) {
    case EventType::k_MouseMoved:
        m_Cursor = event.mouse;
        break;

    case EventType::k_MouseButtonPressed:
        if (event.button == MouseButton::k_Left) {
            const int slot = SlotAt(m_Cursor);
            if (slot >= 0) {
                ClickSlot(slot);
            }
        }
        break;

    case EventType::k_KeyPressed:
        if (!event.isRepeat) {
            // 'E' (the open key) and Escape both close the screen, resuming play.
            if (event.key == Key::k_E || event.key == Key::k_Escape) {
                Close();
            } else if (event.key == Key::k_F3) {
                // F3 keeps toggling the debug overlay while the screen is up.
                Application::Get().ToggleDebugUI();
            }
        }
        break;

    default:
        break;
    }

    // The screen owns every input while it is up, so nothing leaks to the world
    // beneath it.
    event.handled = true;
}

} // namespace Krafter
