#pragma once

#include "Krafter/World/Block.h"

namespace Krafter {

inline constexpr ToolType ToolTypeOf(ItemKind kind)
{
    switch (kind) {
    case ItemKind::k_WoodenAxe:
        return ToolType::k_Axe;
    case ItemKind::k_WoodenPickaxe:
        return ToolType::k_Pickaxe;
    case ItemKind::k_WoodenShovel:
        return ToolType::k_Shovel;
    case ItemKind::k_WoodenSword:
    case ItemKind::k_Coal:
    case ItemKind::k_CopperIngot:
    case ItemKind::k_IronIngot:
        return ToolType::k_None;
    }
    return ToolType::k_None;
}

// Tile in the items atlas (column, row from the top), matching items.png.
inline constexpr glm::vec2 ItemCell(ItemKind kind)
{
    switch (kind) {
    case ItemKind::k_WoodenAxe:
        return glm::vec2(0.0f, 0.0f);
    case ItemKind::k_WoodenPickaxe:
        return glm::vec2(1.0f, 0.0f);
    case ItemKind::k_WoodenShovel:
        return glm::vec2(2.0f, 0.0f);
    case ItemKind::k_WoodenSword:
        return glm::vec2(3.0f, 0.0f);
    case ItemKind::k_Coal:
        return glm::vec2(0.0f, 15.0f);
    case ItemKind::k_CopperIngot:
        return glm::vec2(1.0f, 15.0f);
    case ItemKind::k_IronIngot:
        return glm::vec2(2.0f, 15.0f);
    }
    return glm::vec2(0.0f, 0.0f);
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

    static constexpr Item Material(ItemKind kind, int count = 1)
    {
        Item item;
        item.isItem = true;
        item.kind = kind;
        item.count = count;
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

inline Item DropItemFor(Block broken)
{
    const BlockInfo& info = BlockInfoOf(broken);
    if (info.dropsItem) {
        return Item::Material(info.dropItem);
    }
    return Item(info.drop);
}

}
