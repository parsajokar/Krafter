#pragma once

#include "Krafter/World/Block.h"

namespace Krafter {

// Non-block items: tools and the like. Unlike blocks they have no voxel in the
// world and cannot be placed; they only ever live in inventory and hotbar slots.
enum class ItemKind {
    k_WoodenAxe,
};

// The contents of one hotbar or inventory slot: either a placeable block or a
// standalone item (a tool), or nothing at all. A block slot stores its Block
// (k_Air means empty); an item slot sets isItem and stores the kind. A Block
// converts to an Item implicitly, so the block-only call sites read unchanged.
struct Item {
    // The most of one block a single slot stacks before spilling into the next.
    static constexpr int k_MaxStack = 999;

    Block block = Block::k_Air;
    ItemKind kind = ItemKind::k_WoodenAxe;
    bool isItem = false;
    // How many are in this slot. Zero for an empty slot; a filled block or tool
    // slot holds at least one, blocks stacking up to k_MaxStack.
    int count = 0;

    constexpr Item() = default;
    constexpr Item(Block block)
        : block(block)
        , count(block == Block::k_Air ? 0 : 1)
    {
    }

    static constexpr Item Tool(ItemKind kind)
    {
        Item item;
        item.isItem = true;
        item.kind = kind;
        item.count = 1;
        return item;
    }

    constexpr bool IsEmpty() const
    {
        return !isItem && block == Block::k_Air;
    }

    // A placeable block (a non-empty block slot). Tools and empty slots are not.
    constexpr bool IsBlock() const
    {
        return !isItem && block != Block::k_Air;
    }

    constexpr bool operator==(const Item& other) const
    {
        return isItem == other.isItem
            && (isItem ? kind == other.kind : block == other.block);
    }
    constexpr bool operator!=(const Item& other) const
    {
        return !(*this == other);
    }
};

// Whether `item` is the right tool to break `target`. Tools are picky: a bare
// hand (or a held block) breaks nothing, and each tool only works on the blocks
// it is meant for. The wooden axe fells trees (logs, wood, leaves) and clears
// plants (grass tufts, ferns, dead bushes, and cacti); everything else has no
// tool to break it yet.
inline bool CanBreakWith(const Item& item, Block target)
{
    if (!item.isItem) {
        return false;
    }
    switch (item.kind) {
    case ItemKind::k_WoodenAxe:
        return IsTreePart(target) || IsPlant(target) || target == Block::k_Cactus;
    }
    return false;
}

} // namespace Krafter
