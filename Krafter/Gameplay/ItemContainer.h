#pragma once

#include <cassert>
#include <initializer_list>
#include <vector>

#include "Krafter/World/Item.h"

namespace Krafter {

class ItemContainer {
public:
    explicit ItemContainer(int slotCount)
        : m_Slots(slotCount)
    {
    }

    int GetSlotCount() const
    {
        return static_cast<int>(m_Slots.size());
    }

    Item GetItem(int slot) const
    {
        return m_Slots[slot];
    }

    void SetItem(int slot, Item item)
    {
        assert(item.count <= item.MaxStack() && "a slot can't hold more than one full stack");
        m_Slots[slot] = item;
    }

private:
    std::vector<Item> m_Slots;
};

class ItemSlots {
public:
    ItemSlots(std::initializer_list<ItemContainer*> containers)
        : m_Containers(containers)
    {
    }

    int GetSlotCount() const;
    Item GetItem(int slot) const;
    void SetItem(int slot, Item item);

private:
    std::vector<ItemContainer*> m_Containers;
};

int RoomFor(const ItemSlots& slots, const Item& item);

Item TryInsert(ItemSlots& slots, Item item);

void ClickSlot(ItemSlots& slots, int slot, Item& cursor);

}
