#include <algorithm>

#include "Krafter/Gameplay/ItemContainer.h"

namespace Krafter {

int ItemSlots::GetSlotCount() const
{
    int count = 0;
    for (const ItemContainer* container : m_Containers) {
        count += container->GetSlotCount();
    }
    return count;
}

Item ItemSlots::GetItem(int slot) const
{
    for (const ItemContainer* container : m_Containers) {
        if (slot < container->GetSlotCount()) {
            return container->GetItem(slot);
        }
        slot -= container->GetSlotCount();
    }
    return Item();
}

void ItemSlots::SetItem(int slot, Item item)
{
    for (ItemContainer* container : m_Containers) {
        if (slot < container->GetSlotCount()) {
            container->SetItem(slot, item);
            return;
        }
        slot -= container->GetSlotCount();
    }
}

int RoomFor(const ItemSlots& slots, const Item& item)
{
    int room = 0;
    const int slotCount = slots.GetSlotCount();
    for (int i = 0; i < slotCount && room < item.count; i++) {
        const Item stack = slots.GetItem(i);
        if (stack.IsEmpty()) {
            room += item.MaxStack();
        } else if (stack.StacksWith(item)) {
            room += std::max(0, stack.MaxStack() - stack.count);
        }
    }
    return std::min(room, item.count);
}

Item TryInsert(ItemSlots& slots, Item item)
{
    const int slotCount = slots.GetSlotCount();

    for (int i = 0; i < slotCount && item.count > 0; i++) {
        Item stack = slots.GetItem(i);
        if (!stack.StacksWith(item) || stack.count >= stack.MaxStack()) {
            continue;
        }
        const int moved = std::min(stack.MaxStack() - stack.count, item.count);
        stack.count += moved;
        item.count -= moved;
        slots.SetItem(i, stack);
    }

    for (int i = 0; i < slotCount && item.count > 0; i++) {
        if (!slots.GetItem(i).IsEmpty()) {
            continue;
        }
        Item placed = item;
        placed.count = std::min(item.count, item.MaxStack());
        item.count -= placed.count;
        slots.SetItem(i, placed);
    }

    return item.count > 0 ? item : Item();
}

void ClickSlot(ItemSlots& slots, int slot, Item& cursor)
{
    Item stack = slots.GetItem(slot);

    if (stack.StacksWith(cursor) && stack.count < stack.MaxStack()) {
        const int moved = std::min(stack.MaxStack() - stack.count, cursor.count);
        stack.count += moved;
        cursor.count -= moved;
        slots.SetItem(slot, stack);
        if (cursor.count <= 0) {
            cursor = Item();
        }
        return;
    }

    slots.SetItem(slot, cursor);
    cursor = stack;
}

}
