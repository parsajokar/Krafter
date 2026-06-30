#pragma once

#include <functional>
#include <vector>

#include "glm/glm.hpp"

#include "Krafter/Item.h"
#include "Krafter/Renderer/Texture.h"
#include "Krafter/UIScreen.h"

namespace Krafter {

class Window;
class UIRenderer;
class Font;
class Inventory;
class Hotbar;
struct Recipe;

// The inventory screen, shown over the world (and HUD) when 'E' is pressed. Dims
// the scene and lays out the player's storage: a 3x10 grid of inventory slots
// with the 10-slot hotbar row beneath it, and a horizontally scrolling crafting
// bar (Terraria-style) of recipes below that. 'E' or Escape closes it. The
// callback runs the close (resuming play), leaving the layer change to the owner.
class InventoryLayer : public UIScreen {
public:
    // The UI renderer, sprite sheet (ui.png), and font are owned by the
    // application and shared across the UI layers, so the screen only borrows
    // them. The inventory and hotbar are the player's, shared by reference.
    InventoryLayer(
        Window& window, UIRenderer& renderer, Texture2D& uiTexture, Font& font,
        Inventory& inventory, Hotbar& hotbar, std::function<void()> onClose);

private:
    void OnUpdate() override;
    void OnRender() override;
    void OnEvent(Event& event) override;

    // Top-left corner of the whole panel (inventory grid plus hotbar row),
    // centred in the window and recomputed each frame so it tracks resizes.
    glm::vec2 PanelOrigin() const;

    // The pixel rectangle (xy = top-left, zw = size) of slot `index`.
    glm::vec4 SlotRect(int index) const;

    // The slot index under `point`, or -1 if it is over no slot. Spans both the
    // grid and the hotbar row.
    int SlotAt(const glm::vec2& point) const;

    // The item in / written to slot `index`, mapping the unified index onto the
    // inventory grid or the hotbar row behind it.
    Item GetSlot(int index) const;
    void SetSlot(int index, Item item);

    // Picks up, drops, or swaps the held item against the clicked slot, the way
    // a left-click does in Minecraft's inventory.
    void ClickSlot(int index);

    // The recipes shown in the crafting bar: only those the player has at least
    // some ingredient for, in their fixed order. The bar's contents follow what's
    // in the inventory, the way Terraria lists only the recipes you can start.
    std::vector<const Recipe*> AvailableRecipes() const;

    // The crafting bar (Terraria-style): a strip of recipe-result slots that
    // scrolls behind a fixed selection frame in its middle. The selected recipe is
    // the one resting under that frame; clicking another slot scrolls it there.
    //   CraftBarRect      - the whole strip, wider than the panel, fading at edges.
    //   SelectionSlotRect - the fixed centre frame the selected recipe sits in.
    //   RecipeRect        - the `displayIndex`-th shown slot (scroll applied).
    //   RecipeAt          - the shown recipe under `point` (-1 if none/too faded).
    //   TargetScroll      - the scroll that centres the selected recipe.
    glm::vec4 CraftBarRect() const;
    glm::vec4 SelectionSlotRect() const;
    glm::vec4 RecipeRect(int displayIndex) const;
    int RecipeAt(const glm::vec2& point) const;
    float TargetScroll() const;

    // How visible the recipe with rect `recipeRect` is (1 in the middle of the
    // bar, ramping to 0 at the faded edges), used both to draw it and to ignore
    // clicks on the near-invisible slots at the ends.
    float RecipeFade(const glm::vec4& recipeRect) const;

    // Total count of `item` (matching kind) across every storage slot.
    int CountItem(const Item& item) const;

    // Removes `count` of `item` from storage, draining partial stacks in order.
    void ConsumeItem(const Item& item, int count);

    // Whether `recipe` can be crafted right now (the ingredients are in storage
    // and the cursor can take the result), and crafting it: consume the inputs and
    // drop the output onto the cursor (m_Held), stacking onto a matching held one.
    bool CanCraft(const Recipe& recipe) const;
    void Craft(const Recipe& recipe);

    // Closes the screen, first returning any held item to a free slot so it is
    // not lost when this layer (and its held state) is destroyed.
    void Close();

    std::function<void()> m_OnClose;

    Inventory& m_Inventory;
    Hotbar& m_Hotbar;

    // The block and item atlases, the screen's own, used only for the slot icons
    // (mirrors the HUD, which keeps its own copies for the same reason).
    Texture2D m_BlockTexture;
    Texture2D m_ItemTexture;

    // The item currently picked up onto the cursor (empty when none); it follows
    // the pointer (m_Cursor, in UIScreen) and is returned to a slot on close.
    Item m_Held;

    // The recipe resting under the crafting bar's selection frame (an index into
    // the shown recipes), and the bar's current scroll in pixels, eased toward the
    // scroll that centres the selected recipe so it glides into place.
    int m_SelectedRecipe = 0;
    float m_CraftScroll = 0.0f;
    // Cleared until the first frame snaps the scroll to the selection, so the bar
    // doesn't visibly slide into place when the screen first opens.
    bool m_CraftScrollReady = false;

    // Holding the mouse on the selected recipe keeps crafting it: the recipe being
    // held (null when not crafting) and the time the next repeat is due. Recipes
    // live in a stable list, so the pointer survives the inventory changing.
    const Recipe* m_HeldRecipe = nullptr;
    float m_NextCraftTime = 0.0f;
};

} // namespace Krafter
