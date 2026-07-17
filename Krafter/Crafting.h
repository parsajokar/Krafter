#pragma once

#include <vector>

#include "Krafter/Item.h"
#include "Krafter/World/Block.h"

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
    int count;
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
        return item.IsBlock() && IsPlanks(item.block);
    }
    return false;
}

struct Recipe {
    std::vector<Ingredient> inputs;
    Item output;
    int outputCount;
    CraftingStation station = CraftingStation::k_Hand;
};

inline const std::vector<Recipe>& Recipes()
{
    static const std::vector<Recipe> recipes = {
        { { { Item(Block::k_OakLog), 1 } }, Item(Block::k_OakPlanks), 1 },
        { { { Item(Block::k_BirchLog), 1 } }, Item(Block::k_BirchPlanks), 1 },
        { { { Item(Block::k_AcaciaLog), 1 } }, Item(Block::k_AcaciaPlanks), 1 },

        { { { Item(Block::k_OakPlanks), 1, IngredientMatch::k_AnyPlanks },
              { Item::Material(ItemKind::k_Coal), 1 } },
            Item(Block::k_Torch), 3 },

        { { { Item(Block::k_OakPlanks), 24, IngredientMatch::k_AnyPlanks } },
            Item(Block::k_Workbench), 1 },

        { { { Item(Block::k_Stone), 48 } },
            Item(Block::k_Furnace), 1, CraftingStation::k_Workbench },

        { { { Item(Block::k_OakPlanks), 48, IngredientMatch::k_AnyPlanks } },
            Item::Tool(ItemKind::k_WoodenAxe), 1, CraftingStation::k_Workbench },
        { { { Item(Block::k_OakPlanks), 48, IngredientMatch::k_AnyPlanks } },
            Item::Tool(ItemKind::k_WoodenPickaxe), 1, CraftingStation::k_Workbench },
        { { { Item(Block::k_OakPlanks), 32, IngredientMatch::k_AnyPlanks } },
            Item::Tool(ItemKind::k_WoodenShovel), 1, CraftingStation::k_Workbench },
        { { { Item(Block::k_OakPlanks), 36, IngredientMatch::k_AnyPlanks } },
            Item::Tool(ItemKind::k_WoodenSword), 1, CraftingStation::k_Workbench },

        { { { Item(Block::k_CopperOre), 3 }, { Item::Material(ItemKind::k_Coal), 1 } },
            Item::Material(ItemKind::k_CopperIngot), 1, CraftingStation::k_Furnace },
        { { { Item(Block::k_IronOre), 3 }, { Item::Material(ItemKind::k_Coal), 1 } },
            Item::Material(ItemKind::k_IronIngot), 1, CraftingStation::k_Furnace },
    };
    return recipes;
}

}
