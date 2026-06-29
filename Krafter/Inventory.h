#pragma once

#include <array>

#include "Krafter/World/Block.h"

namespace Krafter {

// The player's main storage: the grid of block slots shown in the inventory
// screen, sitting above the hotbar. A flat grid (k_Rows x k_Columns) the player
// owns; the inventory overlay reads it by reference, keeping it decoupled from
// the player. The hotbar (see Hotbar) is a separate row of quick-select slots.
class Inventory {
public:
    static constexpr int k_Columns = 10;
    static constexpr int k_Rows = 3;
    static constexpr int k_SlotCount = k_Columns * k_Rows;

    Block GetBlock(int slot) const
    {
        return m_Slots[slot];
    }

    void SetBlock(int slot, Block block)
    {
        m_Slots[slot] = block;
    }

private:
    // Every slot starts empty (k_Air).
    std::array<Block, k_SlotCount> m_Slots = {};
};

} // namespace Krafter
