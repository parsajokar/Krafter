#pragma once

#include <functional>

#include "glm/glm.hpp"

#include "Krafter/Core/Layer.h"
#include "Krafter/Item.h"
#include "Krafter/Renderer/Font.h"
#include "Krafter/Renderer/Texture.h"
#include "Krafter/Renderer/UIRenderer.h"

namespace Krafter {

class Window;
class Inventory;
class Hotbar;

// The inventory screen, shown over the world (and HUD) when 'E' is pressed. Dims
// the scene and lays out the player's storage: a 3x10 grid of inventory slots
// with the 10-slot hotbar row beneath it. 'E' or Escape closes it. The callback
// runs the close (resuming play), leaving the layer-stack change to the owner.
class InventoryLayer : public Layer {
public:
    // The UI renderer, sprite sheet (ui.png), and font are owned by the
    // application and shared across the UI layers, so the screen only borrows
    // them. The inventory and hotbar are the player's, shared by reference.
    InventoryLayer(
        Window& window, UIRenderer& renderer, Texture2D& uiTexture, Font& font,
        Inventory& inventory, Hotbar& hotbar, std::function<void()> onClose);

private:
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

    // Closes the screen, first returning any held item to a free slot so it is
    // not lost when this layer (and its held state) is destroyed.
    void Close();

    Window& m_Window;
    std::function<void()> m_OnClose;

    UIRenderer& m_Renderer;
    Texture2D& m_UITexture;
    Font& m_Font;

    Inventory& m_Inventory;
    Hotbar& m_Hotbar;

    // The block and item atlases, the screen's own, used only for the slot icons
    // (mirrors the HUD, which keeps its own copies for the same reason).
    Texture2D m_BlockTexture;
    Texture2D m_ItemTexture;

    // The item currently picked up onto the cursor (empty when none), and the
    // last known cursor position so it can be drawn following the pointer.
    Item m_Held;
    glm::vec2 m_Cursor = glm::vec2(0.0f);
};

} // namespace Krafter
