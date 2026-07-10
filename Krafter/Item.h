#pragma once

#include "Krafter/World/Block.h"

namespace Krafter {

enum class ItemKind {
    k_WoodenAxe,
    k_WoodenPickaxe,
    k_WoodenShovel,
};

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

struct Item {
    static constexpr int k_MaxStack = 999;

    Block block = Block::k_Air;
    ItemKind kind = ItemKind::k_WoodenAxe;
    bool isItem = false;
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

inline bool CanBreakWith(const Item& item, Block target)
{
    if (!item.isItem) {
        return IsPlant(target);
    }
    return CanHarvestWith(target, ToolTypeOf(item.kind));
}

}
