#include <algorithm>
#include <cmath>
#include <string_view>
#include <utility>

#include "Krafter/Core/Application.h"
#include "Krafter/Core/Event.h"
#include "Krafter/Core/Window.h"
#include "Krafter/Crafting.h"
#include "Krafter/Hotbar.h"
#include "Krafter/Inventory.h"
#include "Krafter/InventoryLayer.h"
#include "Krafter/Renderer/Font.h"
#include "Krafter/Renderer/UIRenderer.h"
#include "Krafter/Renderer/Widgets.h"

namespace Krafter {

namespace {

constexpr glm::vec2 k_SpriteSize = glm::vec2(22.0f, 22.0f);
constexpr float k_Scale = 2.0f;
constexpr float k_Spacing = 2.0f;

constexpr float k_HotbarGap = 12.0f;

constexpr float k_CraftGap = 16.0f;
constexpr float k_CraftLabelHeight = 16.0f;
constexpr float k_CraftLabelGap = 4.0f;
constexpr std::string_view k_CraftLabel = "Crafting";
constexpr float k_CraftLabelScale = 1.0f;

constexpr std::string_view k_EmptyCraftText = "Collect materials to craft new stuff!";
constexpr float k_EmptyCraftScale = 1.0f;
constexpr glm::vec4 k_EmptyCraftColor = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);

constexpr float k_ScrollEase = 0.25f;

constexpr float k_CraftHoldDelay = 0.4f;
constexpr float k_CraftHoldInterval = 0.12f;

constexpr float k_RecipeLockedSlotDim = 0.45f;
constexpr float k_RecipeLockedIconDim = 0.35f;

constexpr float k_SlotOpacity = 0.6f;
constexpr glm::vec4 k_DimColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.6f);

constexpr std::string_view k_Title = "Inventory";
constexpr float k_TitleScale = 2.0f;
constexpr float k_TitleGap = 16.0f;

constexpr float k_SlotSize = k_SpriteSize.x * k_Scale;
constexpr float k_Stride = k_SlotSize + k_Spacing * k_Scale;

constexpr float k_CraftBarExtend = 2.0f * k_Stride;
constexpr float k_CraftFadeWidth = 1.5f * k_Stride;

constexpr float k_RecipeClickFade = 0.5f;

constexpr float k_IngredientGap = 8.0f;
constexpr float k_ArrowHeight = 18.0f;
constexpr float k_ArrowWidth = 20.0f;
constexpr glm::vec4 k_ArrowColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.85f);
constexpr float k_IngredientArea
    = k_IngredientGap + k_ArrowHeight + k_IngredientGap + k_SlotSize;

constexpr float k_IconInset = 4.0f * k_Scale;
constexpr float k_IconScale = 0.87f;
constexpr float k_InsetSize = k_SlotSize - 2.0f * k_IconInset;
constexpr float k_IconSize = k_InsetSize * k_IconScale;
constexpr float k_IconOffset = k_IconInset + (k_InsetSize - k_IconSize) * 0.5f;

constexpr float k_PanelWidth = k_Stride * Inventory::k_Columns - k_Spacing * k_Scale;
constexpr float k_GridHeight = k_Stride * Inventory::k_Rows - k_Spacing * k_Scale;
constexpr float k_CraftBarTop = k_GridHeight + k_HotbarGap + k_SlotSize + k_CraftGap
    + k_CraftLabelHeight + k_CraftLabelGap;
constexpr float k_PanelHeight = k_CraftBarTop + k_SlotSize + k_IngredientArea;

constexpr int k_GridSlots = Inventory::k_SlotCount;
constexpr int k_TotalSlots = k_GridSlots + Hotbar::k_SlotCount;

void DrawUpArrow(UIRenderer& renderer, const glm::vec2& tip, const glm::vec4& color)
{
    constexpr int k_HeadRows = 5;
    const float headHeight = k_ArrowHeight * 0.55f;
    const float rowHeight = std::ceil(headHeight / k_HeadRows);
    for (int i = 0; i < k_HeadRows; ++i) {
        const float t = static_cast<float>(i + 1) / k_HeadRows;
        const float width = glm::floor(k_ArrowWidth * t);
        const glm::vec2 pos = glm::floor(glm::vec2(tip.x - width * 0.5f, tip.y + i * rowHeight));
        renderer.DrawQuad(pos, glm::vec2(width, rowHeight), color);
    }
    const float shaftWidth = glm::floor(k_ArrowWidth * 0.34f);
    const glm::vec2 shaftPos = glm::floor(glm::vec2(tip.x - shaftWidth * 0.5f, tip.y + headHeight));
    renderer.DrawQuad(shaftPos, glm::vec2(shaftWidth, k_ArrowHeight - headHeight), color);
}

}

InventoryLayer::InventoryLayer(
    Window& window, UIRenderer& renderer, Texture2D& uiTexture, Font& font,
    Inventory& inventory, Hotbar& hotbar, bool nearWorkbench, bool nearFurnace,
    std::function<void()> onClose)
    : UIScreen("Inventory", window, renderer, uiTexture, font)
    , m_OnClose(std::move(onClose))
    , m_Inventory(inventory)
    , m_Hotbar(hotbar)
    , m_NearWorkbench(nearWorkbench)
    , m_NearFurnace(nearFurnace)
    , m_BlockTexture("assets/textures/blocks.png")
    , m_ItemTexture("assets/textures/items.png")
{
}

glm::vec2 InventoryLayer::PanelOrigin() const
{
    const glm::vec2 windowSize = glm::vec2(m_Window.GetSize());
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

    SetSlot(index, m_Held);
    m_Held = slotItem;
}

std::vector<const Recipe*> InventoryLayer::AvailableRecipes() const
{
    std::vector<const Recipe*> available;
    for (const Recipe& recipe : Recipes()) {
        if (recipe.station == CraftingStation::k_Workbench && !m_NearWorkbench) {
            continue;
        }
        if (recipe.station == CraftingStation::k_Furnace && !m_NearFurnace) {
            continue;
        }
        for (const Ingredient& ingredient : recipe.inputs) {
            if (CountIngredient(ingredient) > 0) {
                available.push_back(&recipe);
                break;
            }
        }
    }
    return available;
}

glm::vec4 InventoryLayer::CraftBarRect() const
{
    const glm::vec2 origin = PanelOrigin();
    return glm::vec4(origin.x - k_CraftBarExtend, origin.y + k_CraftBarTop,
        k_PanelWidth + 2.0f * k_CraftBarExtend, k_SlotSize);
}

glm::vec4 InventoryLayer::SelectionSlotRect() const
{
    const glm::vec2 origin = PanelOrigin();
    return glm::vec4(origin.x + (k_PanelWidth - k_SlotSize) * 0.5f,
        origin.y + k_CraftBarTop, k_SlotSize, k_SlotSize);
}

float InventoryLayer::RecipeFade(const glm::vec4& recipeRect) const
{
    const glm::vec4 bar = CraftBarRect();
    const float center = recipeRect.x + recipeRect.z * 0.5f;
    const float fromLeft = (center - bar.x) / k_CraftFadeWidth;
    const float fromRight = (bar.x + bar.z - center) / k_CraftFadeWidth;
    return glm::clamp(std::min(fromLeft, fromRight), 0.0f, 1.0f);
}

glm::vec4 InventoryLayer::RecipeRect(int displayIndex) const
{
    const glm::vec2 origin = PanelOrigin();
    return glm::vec4(origin.x + displayIndex * k_Stride - m_CraftScroll,
        origin.y + k_CraftBarTop, k_SlotSize, k_SlotSize);
}

int InventoryLayer::RecipeAt(const glm::vec2& point) const
{
    if (!RectContains(CraftBarRect(), point)) {
        return -1;
    }
    const int shown = static_cast<int>(AvailableRecipes().size());
    for (int i = 0; i < shown; ++i) {
        const glm::vec4 rect = RecipeRect(i);
        if (RectContains(rect, point) && RecipeFade(rect) >= k_RecipeClickFade) {
            return i;
        }
    }
    return -1;
}

float InventoryLayer::TargetScroll() const
{
    return m_SelectedRecipe * k_Stride + k_SlotSize * 0.5f - k_PanelWidth * 0.5f;
}

int InventoryLayer::CountIngredient(const Ingredient& ingredient) const
{
    int total = 0;
    for (int i = 0; i < k_TotalSlots; ++i) {
        const Item slot = GetSlot(i);
        if (MatchesIngredient(ingredient, slot)) {
            total += slot.count;
        }
    }
    return total;
}

void InventoryLayer::ConsumeIngredient(const Ingredient& ingredient, int count)
{
    for (int i = 0; i < k_TotalSlots && count > 0; ++i) {
        Item slot = GetSlot(i);
        if (!MatchesIngredient(ingredient, slot)) {
            continue;
        }
        const int taken = std::min(count, slot.count);
        slot.count -= taken;
        count -= taken;
        SetSlot(i, slot.count > 0 ? slot : Item());
    }
}

bool InventoryLayer::CanCraft(const Recipe& recipe) const
{
    for (const Ingredient& ingredient : recipe.inputs) {
        if (CountIngredient(ingredient) < ingredient.count) {
            return false;
        }
    }
    return m_Held.IsEmpty()
        || (m_Held == recipe.output && m_Held.count + recipe.outputCount <= Item::k_MaxStack);
}

void InventoryLayer::Craft(const Recipe& recipe)
{
    if (!CanCraft(recipe)) {
        return;
    }
    for (const Ingredient& ingredient : recipe.inputs) {
        ConsumeIngredient(ingredient, ingredient.count);
    }
    if (m_Held.IsEmpty()) {
        m_Held = recipe.output;
        m_Held.count = recipe.outputCount;
    } else {
        m_Held.count += recipe.outputCount;
    }
}

void InventoryLayer::Close()
{
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

void InventoryLayer::OnUpdate()
{
    if (m_HeldRecipe == nullptr) {
        return;
    }
    const float now = m_Window.GetTime();
    if (now < m_NextCraftTime) {
        return;
    }
    if (!CanCraft(*m_HeldRecipe)) {
        m_HeldRecipe = nullptr;
        return;
    }
    Craft(*m_HeldRecipe);
    m_NextCraftTime = now + k_CraftHoldInterval;
}

void InventoryLayer::OnRender()
{
    m_Renderer.Begin();

    m_Renderer.DrawQuad(glm::vec2(0.0f), glm::vec2(m_Window.GetSize()), k_DimColor);

    const glm::vec2 origin = PanelOrigin();

    const float titleWidth = m_Font.Measure(k_Title, k_TitleScale);
    const glm::vec2 titlePos = glm::floor(glm::vec2(
        origin.x + (k_PanelWidth - titleWidth) * 0.5f,
        origin.y - k_TitleGap - m_Font.LineHeight(k_TitleScale)));
    m_Font.Draw(m_Renderer, k_Title, titlePos, k_TitleScale, glm::vec4(1.0f));

    const int hovered = SlotAt(m_Cursor);

    for (int i = 0; i < k_TotalSlots; ++i) {
        const glm::vec2 position = glm::vec2(SlotRect(i));
        DrawSlot(m_Renderer, m_UITexture, position, glm::vec2(k_SlotSize),
            glm::vec4(1.0f, 1.0f, 1.0f, k_SlotOpacity));

        if (i == hovered) {
            DrawSlotOutline(m_Renderer, m_UITexture, position, glm::vec2(k_SlotSize));
        }

        const Item item = GetSlot(i);
        if (!item.IsEmpty()) {
            DrawItemIcon(m_Renderer, m_BlockTexture, m_ItemTexture, item,
                position + glm::vec2(k_IconOffset), glm::vec2(k_IconSize));
            DrawItemCount(m_Renderer, m_Font, item.count, position, glm::vec2(k_SlotSize));
        }
    }

    const glm::vec4 craftBar = CraftBarRect();
    const glm::vec2 labelPos = glm::floor(glm::vec2(
        origin.x, craftBar.y - k_CraftLabelGap - m_Font.LineHeight(k_CraftLabelScale)));
    m_Font.Draw(m_Renderer, k_CraftLabel, labelPos, k_CraftLabelScale, glm::vec4(1.0f));

    const std::vector<const Recipe*> recipes = AvailableRecipes();

    if (recipes.empty()) {
        m_SelectedRecipe = 0;
    } else {
        m_SelectedRecipe = std::clamp(m_SelectedRecipe, 0, static_cast<int>(recipes.size()) - 1);
    }
    const float target = TargetScroll();
    if (!m_CraftScrollReady) {
        m_CraftScroll = target;
        m_CraftScrollReady = true;
    }
    m_CraftScroll += (target - m_CraftScroll) * k_ScrollEase;
    if (std::fabs(target - m_CraftScroll) < 0.5f) {
        m_CraftScroll = target;
    }

    m_Renderer.SetScissor(glm::vec2(craftBar), glm::vec2(craftBar.z, craftBar.w));
    for (int i = 0; i < static_cast<int>(recipes.size()); ++i) {
        const Recipe& recipe = *recipes[i];
        const glm::vec4 rect = RecipeRect(i);
        const glm::vec2 position = glm::vec2(rect);

        const float fade = RecipeFade(rect);
        if (fade <= 0.0f) {
            continue;
        }

        const bool craftable = CanCraft(recipe);
        const float slotDim = craftable ? 1.0f : k_RecipeLockedSlotDim;

        DrawSlot(m_Renderer, m_UITexture, position, glm::vec2(k_SlotSize),
            glm::vec4(glm::vec3(slotDim), k_SlotOpacity * fade));

        DrawItemIcon(m_Renderer, m_BlockTexture, m_ItemTexture, recipe.output,
            position + glm::vec2(k_IconOffset), glm::vec2(k_IconSize), fade);
        DrawItemCount(m_Renderer, m_Font, recipe.outputCount, position, glm::vec2(k_SlotSize), fade);
    }
    m_Renderer.ClearScissor();

    if (!recipes.empty()) {
        const Recipe& selected = *recipes[m_SelectedRecipe];
        const glm::vec4 frame = SelectionSlotRect();
        DrawSlotOutline(m_Renderer, m_UITexture, glm::vec2(frame), glm::vec2(k_SlotSize));

        const float frameCenterX = frame.x + k_SlotSize * 0.5f;
        const glm::vec2 arrowTip = glm::vec2(
            frameCenterX, frame.y + k_SlotSize + k_IngredientGap);
        DrawUpArrow(m_Renderer, arrowTip, k_ArrowColor);

        const int count = static_cast<int>(selected.inputs.size());
        const float rowWidth = count * k_Stride - k_Spacing * k_Scale;
        const float rowX = frameCenterX - rowWidth * 0.5f;
        const float rowY
            = frame.y + k_SlotSize + k_IngredientGap + k_ArrowHeight + k_IngredientGap;
        for (int i = 0; i < count; ++i) {
            const Ingredient& ingredient = selected.inputs[i];
            const glm::vec2 pos = glm::floor(glm::vec2(rowX + i * k_Stride, rowY));
            const bool haveEnough = CountIngredient(ingredient) >= ingredient.count;
            const float slotDim = haveEnough ? 1.0f : k_RecipeLockedSlotDim;
            const float iconDim = haveEnough ? 1.0f : k_RecipeLockedIconDim;

            DrawSlot(m_Renderer, m_UITexture, pos, glm::vec2(k_SlotSize),
                glm::vec4(glm::vec3(slotDim), k_SlotOpacity));
            DrawItemIcon(m_Renderer, m_BlockTexture, m_ItemTexture, ingredient.item,
                pos + glm::vec2(k_IconOffset), glm::vec2(k_IconSize), iconDim);
            DrawItemCount(m_Renderer, m_Font, ingredient.count, pos,
                glm::vec2(k_SlotSize), iconDim);
        }
    } else {
        const glm::vec2 textPos = glm::floor(glm::vec2(
            origin.x,
            craftBar.y + (k_SlotSize - m_Font.LineHeight(k_EmptyCraftScale)) * 0.5f));
        m_Font.Draw(m_Renderer, k_EmptyCraftText, textPos, k_EmptyCraftScale, k_EmptyCraftColor);
    }

    if (!m_Held.IsEmpty()) {
        const glm::vec2 heldPos = m_Cursor - glm::vec2(k_IconSize * 0.5f);
        DrawItemIcon(m_Renderer, m_BlockTexture, m_ItemTexture, m_Held,
            heldPos, glm::vec2(k_IconSize));
        const glm::vec2 slotPos = heldPos - glm::vec2(k_IconOffset);
        DrawItemCount(m_Renderer, m_Font, m_Held.count, slotPos, glm::vec2(k_SlotSize));
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
            } else {
                const int recipe = RecipeAt(m_Cursor);
                if (recipe >= 0) {
                    const std::vector<const Recipe*> shown = AvailableRecipes();
                    if (recipe == m_SelectedRecipe && recipe < static_cast<int>(shown.size())) {
                        Craft(*shown[recipe]);
                        m_HeldRecipe = shown[recipe];
                        m_NextCraftTime = m_Window.GetTime() + k_CraftHoldDelay;
                    } else {
                        m_SelectedRecipe = recipe;
                    }
                }
            }
        }
        break;

    case EventType::k_MouseButtonReleased:
        if (event.button == MouseButton::k_Left) {
            m_HeldRecipe = nullptr;
        }
        break;

    case EventType::k_MouseScrolled: {
        const float wheel = event.mouse.x + event.mouse.y;
        const int shown = static_cast<int>(AvailableRecipes().size());
        if (wheel != 0.0f && shown > 0) {
            const int step = wheel > 0.0f ? -1 : 1;
            m_SelectedRecipe = std::clamp(m_SelectedRecipe + step, 0, shown - 1);
        }
        break;
    }

    case EventType::k_KeyPressed:
        if (!event.isRepeat) {
            if (event.key == Key::k_E || event.key == Key::k_Escape) {
                Close();
            } else if (event.key == Key::k_F3) {
                Application::Get().ToggleDebugUI();
            }
        }
        break;

    default:
        break;
    }

    event.handled = true;
}

}
