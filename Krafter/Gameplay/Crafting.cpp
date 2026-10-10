#include <algorithm>

#include "Krafter/Gameplay/Crafting.h"
#include "Krafter/World/WorldAccess.h"

namespace Krafter {

namespace {

void ConsumeIngredient(ItemSlots& slots, const Ingredient& ingredient, int count)
{
    const int slotCount = slots.GetSlotCount();
    for (int i = 0; i < slotCount && count > 0; ++i) {
        Item slot = slots.GetItem(i);
        if (!MatchesIngredient(ingredient, slot)) {
            continue;
        }
        const int taken = std::min(count, slot.count);
        slot.count -= taken;
        count -= taken;
        slots.SetItem(i, slot.count > 0 ? slot : Item());
    }
}

}

CraftingStations FindCraftingStations(const WorldAccess& world, const glm::vec3& position)
{
    constexpr int k_Reach = 4;
    const glm::ivec3 center = glm::ivec3(glm::floor(position));

    CraftingStations stations;
    for (int dx = -k_Reach; dx <= k_Reach; ++dx) {
        for (int dy = -k_Reach; dy <= k_Reach; ++dy) {
            for (int dz = -k_Reach; dz <= k_Reach; ++dz) {
                const Block block = world.GetBlock(center + glm::ivec3(dx, dy, dz));
                stations.workbench = stations.workbench || block == Block::k_Workbench;
                stations.furnace = stations.furnace || block == Block::k_Furnace;
            }
        }
    }
    return stations;
}

Item IngredientIcon(const ItemSlots& slots, const Ingredient& ingredient)
{
    if (ingredient.match != IngredientMatch::k_Exact) {
        const int slotCount = slots.GetSlotCount();
        for (int i = 0; i < slotCount; ++i) {
            const Item slot = slots.GetItem(i);
            if (MatchesIngredient(ingredient, slot)) {
                Item icon = slot;
                icon.count = ingredient.item.count;
                return icon;
            }
        }
    }
    return ingredient.item;
}

int CountIngredient(const ItemSlots& slots, const Ingredient& ingredient)
{
    int total = 0;
    const int slotCount = slots.GetSlotCount();
    for (int i = 0; i < slotCount; ++i) {
        const Item slot = slots.GetItem(i);
        if (MatchesIngredient(ingredient, slot)) {
            total += slot.count;
        }
    }
    return total;
}

std::vector<const Recipe*> AvailableRecipes(const ItemSlots& slots, const CraftingStations& stations)
{
    std::vector<const Recipe*> available;
    for (const Recipe& recipe : Recipes()) {
        if (!stations.Has(recipe.station)) {
            continue;
        }
        for (const Ingredient& ingredient : recipe.inputs) {
            if (CountIngredient(slots, ingredient) > 0) {
                available.push_back(&recipe);
                break;
            }
        }
    }
    return available;
}

bool CanCraft(const ItemSlots& slots, const Recipe& recipe, const Item& cursor)
{
    for (const Ingredient& ingredient : recipe.inputs) {
        if (CountIngredient(slots, ingredient) < ingredient.item.count) {
            return false;
        }
    }
    return cursor.IsEmpty()
        || (cursor.StacksWith(recipe.output) && cursor.count + recipe.output.count <= cursor.MaxStack());
}

bool Craft(ItemSlots& slots, const Recipe& recipe, Item& cursor)
{
    if (!CanCraft(slots, recipe, cursor)) {
        return false;
    }
    for (const Ingredient& ingredient : recipe.inputs) {
        ConsumeIngredient(slots, ingredient, ingredient.item.count);
    }
    if (cursor.IsEmpty()) {
        cursor = recipe.output;
    } else {
        cursor.count += recipe.output.count;
    }
    return true;
}

}
