#pragma once

#include "Krafter/Gameplay/ItemContainer.h"

namespace Krafter {

class Inventory : public ItemContainer {
public:
    static constexpr int k_Columns = 10;
    static constexpr int k_Rows = 3;
    static constexpr int k_SlotCount = k_Columns * k_Rows;

    Inventory()
        : ItemContainer(k_SlotCount)
    {
    }
};

}
