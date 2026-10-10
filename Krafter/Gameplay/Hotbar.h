#pragma once

#include "Krafter/Gameplay/ItemContainer.h"

namespace Krafter {

class Hotbar : public ItemContainer {
public:
    static constexpr int k_SlotCount = 10;

    Hotbar()
        : ItemContainer(k_SlotCount)
    {
        SetItem(0, Item(ItemKind::k_WoodenAxe));
    }

    int GetSelected() const
    {
        return m_Selected;
    }

    void SetSelected(int slot)
    {
        m_Selected = slot;
    }

    Item GetSelectedItem() const
    {
        return GetItem(m_Selected);
    }

private:
    int m_Selected = 0;
};

}
