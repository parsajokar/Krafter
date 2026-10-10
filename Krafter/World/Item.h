#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>

#include "Krafter/World/Block.h"

namespace Krafter {

struct ToolMaterial {
    uint32_t power;
    float speed;
    float atlasRow;
    const char* prefix;
};

struct ToolShape {
    ToolType tool;
    float atlasCol;
    const char* suffix;
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

enum class ItemId : uint16_t { };

inline constexpr ItemId k_NoItem = ItemId(0);
inline constexpr size_t k_ItemIdCount = static_cast<size_t>(Block::k_Count) + static_cast<size_t>(ItemKind::k_Count);

inline constexpr ItemId ItemIdOf(Block block)
{
    return static_cast<ItemId>(block);
}

inline constexpr ItemId ItemIdOf(ItemKind kind)
{
    return static_cast<ItemId>(static_cast<size_t>(Block::k_Count) + static_cast<size_t>(kind));
}

struct ItemInfo {
    Block block = Block::k_Air;
    ItemKind kind = ItemKind::k_Count;
    int maxStack = 999;
    bool isTool = false;
    ToolType tool = ToolType::k_None;
    uint32_t power = BreakPower::k_None;
    float speed = 1.0f;
    glm::vec2 iconCell = glm::vec2(0.0f);
};

inline constexpr std::array<ItemInfo, k_ItemIdCount> k_ItemInfo = [] {
    std::array<ItemInfo, k_ItemIdCount> table = {};
    for (size_t b = 0; b < static_cast<size_t>(Block::k_Count); b++) {
        table[b].block = static_cast<Block>(b);
    }
    for (size_t k = 0; k < static_cast<size_t>(ItemKind::k_Count); k++) {
        const ItemKind kind = static_cast<ItemKind>(k);
        ItemInfo& info = table[static_cast<size_t>(ItemIdOf(kind))];
        info.kind = kind;
        info.iconCell = ItemCell(kind);
        if (IsToolKind(kind)) {
            info.maxStack = 1;
            info.isTool = true;
            info.tool = ShapeOf(kind).tool;
            info.power = MaterialOf(kind).power;
            info.speed = MaterialOf(kind).speed;
        }
    }
    return table;
}();

inline constexpr const ItemInfo& ItemInfoOf(ItemId id)
{
    return k_ItemInfo[static_cast<size_t>(id)];
}

class Item {
public:
    static constexpr int k_MaxStack = 999;

    constexpr Item() = default;

    constexpr explicit Item(Block block, int count = 1)
        : count(block == Block::k_Air ? 0 : count)
        , m_Id(ItemIdOf(block))
    {
    }

    constexpr explicit Item(ItemKind kind, int count = 1)
        : count(count)
        , m_Id(ItemIdOf(kind))
    {
    }

    constexpr ItemId GetId() const
    {
        return m_Id;
    }

    constexpr const ItemInfo& Info() const
    {
        return ItemInfoOf(m_Id);
    }

    constexpr bool IsEmpty() const
    {
        return m_Id == k_NoItem;
    }

    constexpr bool IsBlock() const
    {
        return Info().block != Block::k_Air;
    }

    constexpr Block GetBlock() const
    {
        return Info().block;
    }

    constexpr ItemKind GetKind() const
    {
        return Info().kind;
    }

    constexpr bool IsTool() const
    {
        return Info().isTool;
    }

    constexpr int MaxStack() const
    {
        return Info().maxStack;
    }

    constexpr bool StacksWith(const Item& other) const
    {
        return !IsEmpty() && *this == other && MaxStack() > 1;
    }

    constexpr bool operator==(const Item& other) const
    {
        return m_Id == other.m_Id;
    }
    constexpr bool operator!=(const Item& other) const
    {
        return !(*this == other);
    }

    int count = 0;

private:
    ItemId m_Id = k_NoItem;
};

inline bool CanBreakWith(const Item& item, Block target)
{
    if (!item.IsTool()) {
        return IsPlant(target);
    }
    return CanHarvestWith(target, item.Info().tool) && item.Info().power >= MinBreakPower(target);
}

inline float BreakTimeWith(Block block, const Item& item)
{
    return BreakSeconds(block) / item.Info().speed;
}

inline Item DropItemFor(Block broken)
{
    const BlockInfo& info = BlockInfoOf(broken);
    return info.dropsItem ? Item(info.dropItem) : Item(info.drop);
}

}
