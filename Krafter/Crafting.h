#pragma once

#include <vector>

#include "Krafter/Item.h"
#include "Krafter/World/Block.h"

namespace Krafter {

enum class IngredientMatch {
    k_Exact,
    k_AnyPlanks,
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
};

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

}
