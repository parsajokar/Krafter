#pragma once

#include <vector>

#include "Krafter/Item.h"
#include "Krafter/World/Block.h"

namespace Krafter {

// One ingredient of a recipe: an item kind and how many of it the recipe needs.
struct Ingredient {
    Item item;
    int count;
};

// A crafting recipe: a set of ingredient stacks turned into one result stack.
// Recipes can take several ingredients (and several of each), the way Terraria's
// do; the crafting bar lays the ingredients out beneath the chosen result.
struct Recipe {
    std::vector<Ingredient> inputs;
    Item output;
    int outputCount;
};

// Every recipe the player can craft, laid out in order along the inventory's
// crafting bar. The first three are the real ones: each log species turns into
// two planks of its kind. The rest are throwaway recipes (with assorted
// ingredient counts and mixes of log types) so the bar overflows, its scrolling
// can be seen, and the multi-ingredient layout is exercised.
inline const std::vector<Recipe>& Recipes()
{
    static const std::vector<Recipe> recipes = {
        { { { Item(Block::k_OakLog), 1 } }, Item(Block::k_OakPlanks), 2 },
        { { { Item(Block::k_BirchLog), 1 } }, Item(Block::k_BirchPlanks), 2 },
        { { { Item(Block::k_AcaciaLog), 1 } }, Item(Block::k_AcaciaPlanks), 2 },

        // Dummy recipes (placeholders). Most cost oak logs so they show up once
        // you've chopped some; several mix in birch and acacia to show multiple
        // ingredients and stay dimmed until you have those too.
        { { { Item(Block::k_OakLog), 2 } }, Item(Block::k_OakPlanks), 4 },
        { { { Item(Block::k_OakLog), 1 }, { Item(Block::k_BirchLog), 1 } },
            Item(Block::k_OakPlanks), 2 },
        { { { Item(Block::k_OakLog), 1 }, { Item(Block::k_AcaciaLog), 1 } },
            Item(Block::k_AcaciaPlanks), 2 },
        { { { Item(Block::k_OakLog), 3 } }, Item(Block::k_OakWood), 1 },
        { { { Item(Block::k_OakLog), 2 }, { Item(Block::k_BirchLog), 1 } },
            Item(Block::k_OakLeaves), 6 },
        { { { Item(Block::k_OakLog), 1 }, { Item(Block::k_BirchLog), 1 },
              { Item(Block::k_AcaciaLog), 1 } },
            Item(Block::k_Dirt), 4 },
        { { { Item(Block::k_OakLog), 4 } }, Item(Block::k_Cactus), 2 },
        { { { Item(Block::k_OakLog), 2 }, { Item(Block::k_BirchLog), 2 } },
            Item(Block::k_Sand), 8 },
        { { { Item(Block::k_OakLog), 1 }, { Item(Block::k_AcaciaLog), 2 } },
            Item(Block::k_ShortGrass), 3 },
        { { { Item(Block::k_OakLog), 5 } }, Item(Block::k_Grass), 1 },
        { { { Item(Block::k_OakLog), 2 }, { Item(Block::k_AcaciaLog), 1 } },
            Item(Block::k_BirchLeaves), 6 },
        { { { Item(Block::k_OakLog), 1 } }, Item(Block::k_Fern), 3 },
        { { { Item(Block::k_OakLog), 3 }, { Item(Block::k_BirchLog), 1 } },
            Item(Block::k_BirchPlanks), 2 },
        { { { Item(Block::k_OakLog), 2 }, { Item(Block::k_BirchLog), 1 },
              { Item(Block::k_AcaciaLog), 2 } },
            Item(Block::k_AcaciaLeaves), 4 },
        { { { Item(Block::k_BirchLog), 1 }, { Item(Block::k_AcaciaLog), 1 } },
            Item(Block::k_DeadBush), 3 },
    };
    return recipes;
}

} // namespace Krafter
