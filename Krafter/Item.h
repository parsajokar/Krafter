#pragma once

#include "Krafter/World/Block.h"

namespace Krafter {

// Non-block items: tools and the like. Unlike blocks they have no voxel in the
// world and cannot be placed; they only ever live in inventory and hotbar slots.
enum class ItemKind {
    k_WoodenAxe,
    k_WoodenPickaxe,
    k_WoodenShovel,
};

// The tool category an item acts as, matched against a block's harvest tags. A
// non-tool item (or one with no harvesting role) is k_None and breaks nothing.
inline constexpr ToolType ToolTypeOf(ItemKind kind)
{
    switch (kind) {
    case ItemKind::k_WoodenAxe:
        return ToolType::k_Axe;
    case ItemKind::k_WoodenPickaxe:
        return ToolType::k_Pickaxe;
    case ItemKind::k_WoodenShovel:
        return ToolType::k_Shovel;
    }
    return ToolType::k_None;
}

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

    // A stack of `count` of one block, for seeding slots with more than the single
    // block the Block constructor gives.
    static constexpr Item Blocks(Block block, int count)
    {
        Item item;
        item.block = block;
        item.count = count;
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

// Whether `item` can break `target`. A tool breaks a block whose harvest tags
// include its category (see HarvestTools). Anything that isn't a tool — a bare
// hand or a held block — counts as a punch, which tears out the soft cross-shaped
// plants (short grass, fern, dead bush) but nothing tougher.
inline bool CanBreakWith(const Item& item, Block target)
{
    if (!item.isItem) {
        return IsPlant(target);
    }
    return CanHarvestWith(target, ToolTypeOf(item.kind));
}

} // namespace Krafter
