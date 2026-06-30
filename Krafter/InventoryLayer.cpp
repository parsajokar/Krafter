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

    // The slot sprite is a 22x22 box drawn at 2x like the rest of the HUD; the
    // sprites themselves come from Widgets (DrawSlot/DrawSlotOutline), so only the
    // source size is needed here, to size the slots and their icons.
    constexpr glm::vec2 k_SpriteSize = glm::vec2(22.0f, 22.0f);
    constexpr float k_Scale = 2.0f;
    constexpr float k_Spacing = 2.0f;

    // The gap between the inventory grid and the hotbar row below it, separating the
    // two the way Minecraft's inventory does.
    constexpr float k_HotbarGap = 12.0f;

    // The crafting bar below the hotbar row: a gap, a small section label, then a
    // strip of recipe slots the same height as a storage slot.
    constexpr float k_CraftGap = 16.0f;        // hotbar row to the crafting label
    constexpr float k_CraftLabelHeight = 16.0f; // reserved height for the label
    constexpr float k_CraftLabelGap = 4.0f;    // label to the recipe strip
    constexpr std::string_view k_CraftLabel = "Crafting";
    constexpr float k_CraftLabelScale = 1.0f;

    // Shown in the crafting area when no recipe's ingredients are in the inventory,
    // so the empty bar reads as "nothing to craft yet" rather than looking broken.
    constexpr std::string_view k_EmptyCraftText = "Collect materials to craft new stuff!";
    constexpr float k_EmptyCraftScale = 1.0f;
    constexpr glm::vec4 k_EmptyCraftColor = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);

    // How quickly the bar eases toward centring the selected recipe each frame (a
    // fraction of the remaining distance), so a click glides the recipe into place.
    constexpr float k_ScrollEase = 0.25f;

    // Holding the button on a recipe keeps crafting it: a pause after the first
    // craft, then one more every interval, the way held keys auto-repeat.
    constexpr float k_CraftHoldDelay = 0.4f;
    constexpr float k_CraftHoldInterval = 0.12f;

    // A recipe the player can't currently afford is drawn dimmer: its rounded slot
    // sprite darkened and its icon faded, so it reads as disabled without a flat
    // black square stamped over the slot's rounded corners.
    constexpr float k_RecipeLockedSlotDim = 0.45f;
    constexpr float k_RecipeLockedIconDim = 0.35f;

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

    // The crafting bar reaches past the inventory panel on both sides, and its
    // recipe slots fade out across this margin at each end instead of being cut
    // off, so they slide smoothly in and out as the bar scrolls.
    constexpr float k_CraftBarExtend = 2.0f * k_Stride;
    constexpr float k_CraftFadeWidth = 1.5f * k_Stride;

    // A recipe must be at least this visible (its fade above this) to be clicked,
    // so the all-but-invisible slots at the very edges aren't hit by accident.
    constexpr float k_RecipeClickFade = 0.5f;

    // Below the selected recipe sits its ingredient row, with an arrow pointing up
    // from the ingredients to the result. These size that area and the arrow.
    constexpr float k_IngredientGap = 8.0f; // result->arrow and arrow->ingredient
    constexpr float k_ArrowHeight = 18.0f;
    constexpr float k_ArrowWidth = 20.0f;
    constexpr glm::vec4 k_ArrowColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.85f);
    constexpr float k_IngredientArea
        = k_IngredientGap + k_ArrowHeight + k_IngredientGap + k_SlotSize;

    // The icon inset within a slot, matching the hotbar so the icons line up. The
    // inset box, the icon size, and its offset are all square, so kept as scalars.
    constexpr float k_IconInset = 4.0f * k_Scale;
    constexpr float k_IconScale = 0.87f;
    constexpr float k_InsetSize = k_SlotSize - 2.0f * k_IconInset;
    constexpr float k_IconSize = k_InsetSize * k_IconScale;
    constexpr float k_IconOffset = k_IconInset + (k_InsetSize - k_IconSize) * 0.5f;

    // The panel spans 10 columns; the inventory grid, the hotbar row below it, and
    // the crafting bar below that (label and strip).
    constexpr float k_PanelWidth = k_Stride * Inventory::k_Columns - k_Spacing * k_Scale;
    constexpr float k_GridHeight = k_Stride * Inventory::k_Rows - k_Spacing * k_Scale;
    constexpr float k_CraftBarTop = k_GridHeight + k_HotbarGap + k_SlotSize + k_CraftGap
        + k_CraftLabelHeight + k_CraftLabelGap;
    constexpr float k_PanelHeight = k_CraftBarTop + k_SlotSize + k_IngredientArea;

    // Slot indices [0, k_GridSlots) address the inventory grid; the hotbar row
    // follows at [k_GridSlots, k_TotalSlots).
    constexpr int k_GridSlots = Inventory::k_SlotCount;
    constexpr int k_TotalSlots = k_GridSlots + Hotbar::k_SlotCount;

    // Draws a blocky upward arrow (triangular head over a shaft) with its tip at
    // the top-centre `tip`, built from solid quads since the sheet has no arrow
    // sprite; the chunky pixels suit the rest of the art.
    void DrawUpArrow(UIRenderer& renderer, const glm::vec2& tip, const glm::vec4& color)
    {
        constexpr int k_HeadRows = 5;
        const float headHeight = k_ArrowHeight * 0.55f;
        const float rowHeight = std::ceil(headHeight / k_HeadRows);
        for (int i = 0; i < k_HeadRows; ++i) {
            // The head widens from the tip down to its full width.
            const float t = static_cast<float>(i + 1) / k_HeadRows;
            const float width = glm::floor(k_ArrowWidth * t);
            const glm::vec2 pos = glm::floor(glm::vec2(tip.x - width * 0.5f, tip.y + i * rowHeight));
            renderer.DrawQuad(pos, glm::vec2(width, rowHeight), color);
        }
        const float shaftWidth = glm::floor(k_ArrowWidth * 0.34f);
        const glm::vec2 shaftPos = glm::floor(glm::vec2(tip.x - shaftWidth * 0.5f, tip.y + headHeight));
        renderer.DrawQuad(shaftPos, glm::vec2(shaftWidth, k_ArrowHeight - headHeight), color);
    }

} // namespace

InventoryLayer::InventoryLayer(
    Window& window, UIRenderer& renderer, Texture2D& uiTexture, Font& font,
    Inventory& inventory, Hotbar& hotbar, std::function<void()> onClose)
    : UIScreen("Inventory", window, renderer, uiTexture, font)
    , m_OnClose(std::move(onClose))
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

std::vector<const Recipe*> InventoryLayer::AvailableRecipes() const
{
    std::vector<const Recipe*> available;
    for (const Recipe& recipe : Recipes()) {
        // Listed once the player has any of the ingredients, even if not all of
        // them (or not yet enough); those show but stay dimmed until craftable.
        for (const Ingredient& ingredient : recipe.inputs) {
            if (CountItem(ingredient.item) > 0) {
                available.push_back(&recipe);
                break;
            }
        }
    }
    return available;
}

glm::vec4 InventoryLayer::CraftBarRect() const
{
    // Wider than the panel, extended evenly past both sides and kept centred on it.
    const glm::vec2 origin = PanelOrigin();
    return glm::vec4(origin.x - k_CraftBarExtend, origin.y + k_CraftBarTop,
        k_PanelWidth + 2.0f * k_CraftBarExtend, k_SlotSize);
}

glm::vec4 InventoryLayer::SelectionSlotRect() const
{
    // The fixed selection frame sits in the centre of the panel, on the bar row.
    const glm::vec2 origin = PanelOrigin();
    return glm::vec4(origin.x + (k_PanelWidth - k_SlotSize) * 0.5f,
        origin.y + k_CraftBarTop, k_SlotSize, k_SlotSize);
}

float InventoryLayer::RecipeFade(const glm::vec4& recipeRect) const
{
    // Full opacity in the middle, ramping to zero within the fade margin at each
    // end of the bar, so slots dissolve at the edges rather than being clipped.
    const glm::vec4 bar = CraftBarRect();
    const float center = recipeRect.x + recipeRect.z * 0.5f;
    const float fromLeft = (center - bar.x) / k_CraftFadeWidth;
    const float fromRight = (bar.x + bar.z - center) / k_CraftFadeWidth;
    return glm::clamp(std::min(fromLeft, fromRight), 0.0f, 1.0f);
}

glm::vec4 InventoryLayer::RecipeRect(int displayIndex) const
{
    // Recipes lay out from the inventory panel's left edge; the bar's fade margins
    // reach out past it, so the first recipe is fully visible at scroll zero and
    // only dissolves once scrolled into a margin.
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
        // Faded-out slots at the edges aren't clickable, so a near-invisible
        // recipe can't be crafted by a stray click.
        if (RectContains(rect, point) && RecipeFade(rect) >= k_RecipeClickFade) {
            return i;
        }
    }
    return -1;
}

float InventoryLayer::TargetScroll() const
{
    // The scroll that lines the selected recipe's slot up under the centre frame:
    // RecipeRect(s) centre == panel centre.
    return m_SelectedRecipe * k_Stride + k_SlotSize * 0.5f - k_PanelWidth * 0.5f;
}

int InventoryLayer::CountItem(const Item& item) const
{
    int total = 0;
    for (int i = 0; i < k_TotalSlots; ++i) {
        const Item slot = GetSlot(i);
        if (!slot.IsEmpty() && slot == item) {
            total += slot.count;
        }
    }
    return total;
}

void InventoryLayer::ConsumeItem(const Item& item, int count)
{
    for (int i = 0; i < k_TotalSlots && count > 0; ++i) {
        Item slot = GetSlot(i);
        if (slot.IsEmpty() || slot != item) {
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
    // Every ingredient must be present in full.
    for (const Ingredient& ingredient : recipe.inputs) {
        if (CountItem(ingredient.item) < ingredient.count) {
            return false;
        }
    }
    // The result lands on the cursor, so the cursor must be free or already hold a
    // matching stack with room for more.
    return m_Held.IsEmpty()
        || (m_Held == recipe.output && m_Held.count + recipe.outputCount <= Item::k_MaxStack);
}

void InventoryLayer::Craft(const Recipe& recipe)
{
    if (!CanCraft(recipe)) {
        return;
    }
    for (const Ingredient& ingredient : recipe.inputs) {
        ConsumeItem(ingredient.item, ingredient.count);
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

void InventoryLayer::OnUpdate()
{
    // While the button is held on a recipe, keep crafting it on the repeat timer
    // until the materials run out or the cursor can't take any more.
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

    // Dim the world and HUD behind the screen.
    m_Renderer.DrawQuad(glm::vec2(0.0f), glm::vec2(m_Window.GetSize()), k_DimColor);

    const glm::vec2 origin = PanelOrigin();

    // Title, centred over the panel and sitting a gap above the grid.
    const float titleWidth = m_Font.Measure(k_Title, k_TitleScale);
    const glm::vec2 titlePos = glm::floor(glm::vec2(
        origin.x + (k_PanelWidth - titleWidth) * 0.5f,
        origin.y - k_TitleGap - m_Font.LineHeight(k_TitleScale)));
    m_Font.Draw(m_Renderer, k_Title, titlePos, k_TitleScale, glm::vec4(1.0f));

    // The slot under the cursor, highlighted with the selection outline.
    const int hovered = SlotAt(m_Cursor);

    // Every slot: the box sprite, with its block icon centred inside when filled,
    // and the outline on top when the cursor is over it.
    for (int i = 0; i < k_TotalSlots; ++i) {
        const glm::vec2 position = glm::vec2(SlotRect(i));
        DrawSlot(m_Renderer, m_UITexture, position, glm::vec2(k_SlotSize),
            glm::vec4(1.0f, 1.0f, 1.0f, k_SlotOpacity));

        // The hover outline sits behind the icon and count so the highlight frames
        // the slot without painting over the stack number in its corner.
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

    // The crafting bar: a section label, then a strip of recipe-result slots that
    // scrolls behind a fixed selection frame in the middle. The selected recipe
    // rests under the frame; its ingredients sit below, an arrow pointing up to it.
    const glm::vec4 craftBar = CraftBarRect();
    // The label lines up with the inventory panel, not the wider bar's far edge.
    const glm::vec2 labelPos = glm::floor(glm::vec2(
        origin.x, craftBar.y - k_CraftLabelGap - m_Font.LineHeight(k_CraftLabelScale)));
    m_Font.Draw(m_Renderer, k_CraftLabel, labelPos, k_CraftLabelScale, glm::vec4(1.0f));

    const std::vector<const Recipe*> recipes = AvailableRecipes();

    // Keep the selection in range as the list grows and shrinks, then ease the
    // scroll toward centring it so a new selection glides into the frame.
    if (recipes.empty()) {
        m_SelectedRecipe = 0;
    } else {
        m_SelectedRecipe = std::clamp(m_SelectedRecipe, 0, static_cast<int>(recipes.size()) - 1);
    }
    const float target = TargetScroll();
    if (!m_CraftScrollReady) {
        // Start already centred on the first selection, then ease afterwards.
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

        // Everything in the slot fades together toward the bar's edges so the
        // recipes dissolve in and out rather than being clipped.
        const float fade = RecipeFade(rect);
        if (fade <= 0.0f) {
            continue;
        }

        // Recipes the player can't afford keep their rounded slot but are drawn
        // darker, with a faded icon, to read as disabled.
        const bool craftable = CanCraft(recipe);
        const float slotDim = craftable ? 1.0f : k_RecipeLockedSlotDim;
        const float iconFade = craftable ? fade : fade * k_RecipeLockedIconDim;

        DrawSlot(m_Renderer, m_UITexture, position, glm::vec2(k_SlotSize),
            glm::vec4(glm::vec3(slotDim), k_SlotOpacity * fade));

        DrawItemIcon(m_Renderer, m_BlockTexture, m_ItemTexture, recipe.output,
            position + glm::vec2(k_IconOffset), glm::vec2(k_IconSize), iconFade);
        DrawItemCount(m_Renderer, m_Font, recipe.outputCount, position, glm::vec2(k_SlotSize), iconFade);
    }
    m_Renderer.ClearScissor();

    // The fixed selection frame, and below it the selected recipe's ingredients
    // with an arrow pointing up from them to the result resting in the frame.
    if (!recipes.empty()) {
        const Recipe& selected = *recipes[m_SelectedRecipe];
        const glm::vec4 frame = SelectionSlotRect();
        DrawSlotOutline(m_Renderer, m_UITexture, glm::vec2(frame), glm::vec2(k_SlotSize));

        const float frameCenterX = frame.x + k_SlotSize * 0.5f;
        const glm::vec2 arrowTip = glm::vec2(
            frameCenterX, frame.y + k_SlotSize + k_IngredientGap);
        DrawUpArrow(m_Renderer, arrowTip, k_ArrowColor);

        // The ingredient row, centred under the frame: one slot per ingredient,
        // each dimmed when the player doesn't have enough of it yet.
        const int count = static_cast<int>(selected.inputs.size());
        const float rowWidth = count * k_Stride - k_Spacing * k_Scale;
        const float rowX = frameCenterX - rowWidth * 0.5f;
        const float rowY
            = frame.y + k_SlotSize + k_IngredientGap + k_ArrowHeight + k_IngredientGap;
        for (int i = 0; i < count; ++i) {
            const Ingredient& ingredient = selected.inputs[i];
            const glm::vec2 pos = glm::floor(glm::vec2(rowX + i * k_Stride, rowY));
            const bool haveEnough = CountItem(ingredient.item) >= ingredient.count;
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
        // Nothing craftable yet: a hint just under the "Crafting" label, sitting in
        // the bar row where the recipes would be.
        const glm::vec2 textPos = glm::floor(glm::vec2(
            origin.x,
            craftBar.y + (k_SlotSize - m_Font.LineHeight(k_EmptyCraftScale)) * 0.5f));
        m_Font.Draw(m_Renderer, k_EmptyCraftText, textPos, k_EmptyCraftScale, k_EmptyCraftColor);
    }

    // The held item trails the cursor, drawn last so it sits above everything.
    if (!m_Held.IsEmpty()) {
        const glm::vec2 heldPos = m_Cursor - glm::vec2(k_IconSize * 0.5f);
        DrawItemIcon(m_Renderer, m_BlockTexture, m_ItemTexture, m_Held,
            heldPos, glm::vec2(k_IconSize));
        // Frame the count against the slot-sized box the icon would inset into, so
        // the number tucks into the bottom-right corner exactly as it does in a
        // slot rather than landing centred over the icon.
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
                // Outside the storage slots: clicking the recipe already under the
                // selection frame crafts it; clicking any other recipe selects it,
                // scrolling it into the frame.
                const int recipe = RecipeAt(m_Cursor);
                if (recipe >= 0) {
                    const std::vector<const Recipe*> shown = AvailableRecipes();
                    if (recipe == m_SelectedRecipe && recipe < static_cast<int>(shown.size())) {
                        // Craft now, then keep crafting while the button is held.
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
            // Releasing stops the hold-to-craft repeat.
            m_HeldRecipe = nullptr;
        }
        break;

    case EventType::k_MouseScrolled: {
        // The wheel steps the selection one recipe at a time (up = previous), and
        // the scroll eases it into the frame.
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
