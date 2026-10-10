#pragma once

#include <vector>

#include "glm/glm.hpp"

#include "Krafter/Gameplay/ItemContainer.h"
#include "Krafter/World/Block.h"
#include "Krafter/World/Item.h"

namespace Krafter {

enum class IngredientMatch {
    k_Exact,
    k_AnyPlanks,
};

enum class CraftingStation {
    k_Hand,
    k_Workbench,
    k_Furnace,
};

struct Ingredient {
    Item item;
    IngredientMatch match = IngredientMatch::k_Exact;
};

inline bool MatchesIngredient(const Ingredient& ingredient, const Item& item)
{
    if (item.IsEmpty()) {
        return false;
    }
    switch (ingredient.match) {
    case IngredientMatch::k_Exact:
        return item == ingredient.item;
    case IngredientMatch::k_AnyPlanks:
        return item.IsBlock() && IsPlanks(item.GetBlock());
    }
    return false;
}

struct Recipe {
    std::vector<Ingredient> inputs;
    Item output;
    CraftingStation station = CraftingStation::k_Hand;
};

inline const std::vector<Recipe>& Recipes()
{
    static const std::vector<Recipe> recipes = {
        { { { Item(Block::k_OakLog, 1) } },
            Item(Block::k_OakPlanks, 1) },
        { { { Item(Block::k_BirchLog, 1) } },
            Item(Block::k_BirchPlanks, 1) },
        { { { Item(Block::k_AcaciaLog, 1) } },
            Item(Block::k_AcaciaPlanks, 1) },

        { { { Item(Block::k_OakPlanks, 1), IngredientMatch::k_AnyPlanks },
              { Item(ItemKind::k_Coal, 1) } },
            Item(Block::k_Torch, 3) },

        { { { Item(Block::k_OakPlanks, 24), IngredientMatch::k_AnyPlanks } },
            Item(Block::k_Workbench, 1) },

        { { { Item(Block::k_Stone, 48) } },
            Item(Block::k_Furnace, 1), CraftingStation::k_Workbench },

        { { { Item(Block::k_OakPlanks, 48), IngredientMatch::k_AnyPlanks } },
            Item(ItemKind::k_WoodenAxe, 1), CraftingStation::k_Workbench },
        { { { Item(Block::k_OakPlanks, 48), IngredientMatch::k_AnyPlanks } },
            Item(ItemKind::k_WoodenPickaxe, 1), CraftingStation::k_Workbench },
        { { { Item(Block::k_OakPlanks, 32), IngredientMatch::k_AnyPlanks } },
            Item(ItemKind::k_WoodenShovel, 1), CraftingStation::k_Workbench },
        { { { Item(Block::k_OakPlanks, 36), IngredientMatch::k_AnyPlanks } },
            Item(ItemKind::k_WoodenSword, 1), CraftingStation::k_Workbench },

        { { { Item(Block::k_Stone, 48) } },
            Item(ItemKind::k_StoneAxe, 1), CraftingStation::k_Workbench },
        { { { Item(Block::k_Stone, 48) } },
            Item(ItemKind::k_StonePickaxe, 1), CraftingStation::k_Workbench },
        { { { Item(Block::k_Stone, 32) } },
            Item(ItemKind::k_StoneShovel, 1), CraftingStation::k_Workbench },
        { { { Item(Block::k_Stone, 36) } },
            Item(ItemKind::k_StoneSword, 1), CraftingStation::k_Workbench },

        { { { Item(ItemKind::k_CopperIngot, 48) } },
            Item(ItemKind::k_CopperAxe, 1), CraftingStation::k_Workbench },
        { { { Item(ItemKind::k_CopperIngot, 48) } },
            Item(ItemKind::k_CopperPickaxe, 1), CraftingStation::k_Workbench },
        { { { Item(ItemKind::k_CopperIngot, 32) } },
            Item(ItemKind::k_CopperShovel, 1), CraftingStation::k_Workbench },
        { { { Item(ItemKind::k_CopperIngot, 36) } },
            Item(ItemKind::k_CopperSword, 1), CraftingStation::k_Workbench },

        { { { Item(ItemKind::k_IronIngot, 48) } },
            Item(ItemKind::k_IronAxe, 1), CraftingStation::k_Workbench },
        { { { Item(ItemKind::k_IronIngot, 48) } },
            Item(ItemKind::k_IronPickaxe, 1), CraftingStation::k_Workbench },
        { { { Item(ItemKind::k_IronIngot, 32) } },
            Item(ItemKind::k_IronShovel, 1), CraftingStation::k_Workbench },
        { { { Item(ItemKind::k_IronIngot, 36) } },
            Item(ItemKind::k_IronSword, 1), CraftingStation::k_Workbench },

        { { { Item(Block::k_CopperOre, 3) }, { Item(ItemKind::k_Coal, 1) } },
            Item(ItemKind::k_CopperIngot, 1), CraftingStation::k_Furnace },
        { { { Item(Block::k_IronOre, 3) }, { Item(ItemKind::k_Coal, 1) } },
            Item(ItemKind::k_IronIngot, 1), CraftingStation::k_Furnace },
    };
    return recipes;
}

class WorldAccess;

struct CraftingStations {
    bool workbench = false;
    bool furnace = false;

    bool Has(CraftingStation station) const
    {
        switch (station) {
        case CraftingStation::k_Hand:
            return true;
        case CraftingStation::k_Workbench:
            return workbench;
        case CraftingStation::k_Furnace:
            return furnace;
        }
        return false;
    }
};

CraftingStations FindCraftingStations(const WorldAccess& world, const glm::vec3& position);

Item IngredientIcon(const ItemSlots& slots, const Ingredient& ingredient);

int CountIngredient(const ItemSlots& slots, const Ingredient& ingredient);

std::vector<const Recipe*> AvailableRecipes(const ItemSlots& slots, const CraftingStations& stations);

bool CanCraft(const ItemSlots& slots, const Recipe& recipe, const Item& cursor);

bool Craft(ItemSlots& slots, const Recipe& recipe, Item& cursor);

}
