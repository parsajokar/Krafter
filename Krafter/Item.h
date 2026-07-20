#pragma once

#include <iterator>
#include <string>

#include "Krafter/World/Block.h"

namespace Krafter {

// A tool's identity is (material, shape). ItemKind lists the tools material-major
// (one entry per shape, in k_ToolShapes order), so index arithmetic recovers
// both halves. Adding a whole material is one k_ToolMaterials row plus its
// ItemKind entries; adding a shape is one k_ToolShapes row plus one per material.
struct ToolMaterial {
    uint32_t power; // gates which blocks it can break
    float speed; // break-speed multiplier, independent of power
    float atlasRow;
    const char* prefix; // e.g. "Wooden"
};

struct ToolShape {
    ToolType tool;
    float atlasCol;
    const char* suffix; // e.g. "Pickaxe"
};

inline constexpr ToolMaterial k_ToolMaterials[] = {
    { BreakPower::k_Wood, 1.0f, 0.0f, "Wooden" },
    { BreakPower::k_Stone, 1.5f, 1.0f, "Stone" },
    { BreakPower::k_Copper, 2.0f, 2.0f, "Copper" },
    { BreakPower::k_Iron, 3.0f, 3.0f, "Iron" },
};

inline constexpr ToolShape k_ToolShapes[] = {
    { ToolType::k_Axe, 0.0f, "Axe" },
    { ToolType::k_Pickaxe, 1.0f, "Pickaxe" },
    { ToolType::k_Shovel, 2.0f, "Shovel" },
    { ToolType::k_None, 3.0f, "Sword" },
};

inline constexpr int k_ToolShapeCount = static_cast<int>(std::size(k_ToolShapes));

// Non-tool items (materials/ingots) follow the tools in ItemKind order.
struct MaterialItem {
    glm::vec2 cell;
    const char* name;
};

inline constexpr MaterialItem k_MaterialItems[] = {
    { { 0.0f, 15.0f }, "Coal" },
    { { 1.0f, 15.0f }, "Copper Ingot" },
    { { 2.0f, 15.0f }, "Iron Ingot" },
};

inline constexpr int k_FirstMaterialItem = static_cast<int>(std::size(k_ToolMaterials)) * k_ToolShapeCount;

static_assert(static_cast<int>(ItemKind::k_Count) - k_FirstMaterialItem == static_cast<int>(std::size(k_MaterialItems)),
    "k_MaterialItems must cover every non-tool ItemKind");

inline constexpr bool IsToolKind(ItemKind kind)
{
    return static_cast<int>(kind) < k_FirstMaterialItem;
}

inline constexpr const ToolMaterial& MaterialOf(ItemKind kind)
{
    return k_ToolMaterials[static_cast<int>(kind) / k_ToolShapeCount];
}

inline constexpr const ToolShape& ShapeOf(ItemKind kind)
{
    return k_ToolShapes[static_cast<int>(kind) % k_ToolShapeCount];
}

inline constexpr const MaterialItem& MaterialItemOf(ItemKind kind)
{
    return k_MaterialItems[static_cast<int>(kind) - k_FirstMaterialItem];
}

inline constexpr ToolType ToolTypeOf(ItemKind kind)
{
    return IsToolKind(kind) ? ShapeOf(kind).tool : ToolType::k_None;
}

inline constexpr uint32_t ToolPower(ItemKind kind)
{
    return IsToolKind(kind) ? MaterialOf(kind).power : BreakPower::k_None;
}

inline constexpr float ToolSpeed(ItemKind kind)
{
    return IsToolKind(kind) ? MaterialOf(kind).speed : 1.0f;
}

inline constexpr glm::vec2 ItemCell(ItemKind kind)
{
    return IsToolKind(kind) ? glm::vec2(ShapeOf(kind).atlasCol, MaterialOf(kind).atlasRow)
                            : MaterialItemOf(kind).cell;
}

inline std::string ItemKindName(ItemKind kind)
{
    if (IsToolKind(kind)) {
        return std::string(MaterialOf(kind).prefix) + ' ' + ShapeOf(kind).suffix;
    }
    return MaterialItemOf(kind).name;
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
    if (!CanHarvestWith(target, ToolTypeOf(item.kind))) {
        return false;
    }
    return ToolPower(item.kind) >= MinBreakPower(target);
}

// Break time factoring in both the block's hardness and the held tool's tier.
// Only meaningful when CanBreakWith is true (i.e. the tool actually applies).
inline float BreakTimeWith(Block block, const Item& item)
{
    const float base = BreakSeconds(block);
    if (!item.isItem) {
        return base;
    }
    return base / ToolSpeed(item.kind);
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
