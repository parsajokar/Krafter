#pragma once

#include <array>

#include "Krafter/Item.h"

namespace Krafter {

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

    std::array<Item, k_SlotCount> m_Slots = {
        Item::Tool(ItemKind::k_WoodenAxe)
    };
};

}
