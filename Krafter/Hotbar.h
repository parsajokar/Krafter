#pragma once

#include <array>

#include "Krafter/Item.h"

namespace Krafter {

// The player's hotbar: the items held in each slot and which slot is active.
// Owned by the Player (it is the quick-select slice of the inventory) and shared
// by reference with the HUD that draws it. A slot may hold a block or a tool.
class Hotbar {
public:
    static constexpr int k_SlotCount = 10;

    int GetSelected() const
    {
        return m_Selected;
    }

    void SetSelected(int slot)
    {
        m_Selected = slot;
    }

    Item GetItem(int slot) const
    {
        return m_Slots[slot];
    }

    void SetItem(int slot, Item item)
    {
        m_Slots[slot] = item;
    }

    Item GetSelectedItem() const
    {
        return m_Slots[m_Selected];
    }

private:
    int m_Selected = 0;

    // The first slot holds the wooden axe and the second a stack of torches to
    // light the caves; every other slot starts empty (k_Air).
    std::array<Item, k_SlotCount> m_Slots = {
        Item::Tool(ItemKind::k_WoodenAxe),
        Item::Blocks(Block::k_Torch, 99)
    };
};

} // namespace Krafter
