#pragma once

#include <vector>

#include "Krafter/Item.h"
#include "Krafter/World/Block.h"

namespace Krafter {

// How an ingredient matches inventory items: either the one exact item, or any
// block of a category. k_AnyPlanks lets a recipe accept whatever wood species the
// player has, pooling oak, birch and acacia planks toward the same cost.
enum class IngredientMatch {
    k_Exact,
    k_AnyPlanks,
};

// One ingredient of a recipe: an item kind and how many of it the recipe needs.
// `match` widens what satisfies it; `item` is also the icon shown for the slot.
struct Ingredient {
    Item item;
    int count;
    IngredientMatch match = IngredientMatch::k_Exact;
};

// Whether an inventory `item` satisfies `ingredient` (its exact kind, or any block
// of the ingredient's category). Empty items match nothing.
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

// A crafting recipe: a set of ingredient stacks turned into one result stack.
// Recipes can take several ingredients (and several of each), the way Terraria's
// do; the crafting bar lays the ingredients out beneath the chosen result.
struct Recipe {
    std::vector<Ingredient> inputs;
    Item output;
    int outputCount;
};

// Every recipe the player can craft, laid out in order along the inventory's
// crafting bar. Each log species turns into one plank of its kind, and the wooden
// pickaxe and shovel are crafted from planks of any species.
inline const std::vector<Recipe>& Recipes()
{
    static const std::vector<Recipe> recipes = {
        { { { Item(Block::k_OakLog), 1 } }, Item(Block::k_OakPlanks), 1 },
        { { { Item(Block::k_BirchLog), 1 } }, Item(Block::k_BirchPlanks), 1 },
        { { { Item(Block::k_AcaciaLog), 1 } }, Item(Block::k_AcaciaPlanks), 1 },

        { { { Item(Block::k_OakPlanks), 60, IngredientMatch::k_AnyPlanks } },
            Item::Tool(ItemKind::k_WoodenPickaxe), 1 },
        { { { Item(Block::k_OakPlanks), 50, IngredientMatch::k_AnyPlanks } },
            Item::Tool(ItemKind::k_WoodenShovel), 1 },
    };
    return recipes;
}

} // namespace Krafter
