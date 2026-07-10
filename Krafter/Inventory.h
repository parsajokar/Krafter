#pragma once

#include <array>

#include "Krafter/Item.h"

namespace Krafter {

class Inventory {
public:
    static constexpr int k_Columns = 10;
    static constexpr int k_Rows = 3;
    static constexpr int k_SlotCount = k_Columns * k_Rows;

    Item GetItem(int slot) const
    {
        return m_Slots[slot];
    }

    void SetItem(int slot, Item item)
    {
        m_Slots[slot] = item;
    }

private:
    std::array<Item, k_SlotCount> m_Slots = {};
};

}
