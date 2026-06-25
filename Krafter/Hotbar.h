#pragma once

#include <array>

#include "Krafter/World/Block.h"

namespace Krafter {

// The player's hotbar: the blocks held in each slot and which slot is active.
// Owned by the Player (it is the quick-select slice of what will grow into a
// full inventory) and shared by reference with the HUD that draws it.
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

    Block GetBlock(int slot) const
    {
        return m_Slots[slot];
    }

    Block GetSelectedBlock() const
    {
        return m_Slots[m_Selected];
    }

private:
    int m_Selected = 0;

    // Unspecified slots default to k_Air (empty).
    std::array<Block, k_SlotCount> m_Slots = { Block::k_Grass, Block::k_Sand,
        Block::k_OakLog, Block::k_OakLeaves, Block::k_ShortGrass, Block::k_Fern,
        Block::k_DeadBush, Block::k_Cactus };
};

} // namespace Krafter
