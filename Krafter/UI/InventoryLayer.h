#pragma once

#include <functional>
#include <vector>

#include "glm/glm.hpp"

#include "Krafter/Engine/Graphics/Texture.h"
#include "Krafter/Gameplay/Crafting.h"
#include "Krafter/Gameplay/ItemContainer.h"
#include "Krafter/UI/UIScreen.h"
#include "Krafter/World/Item.h"

namespace Krafter {

class Window;
class UIRenderer;
class Font;
class Inventory;
class Hotbar;

class InventoryLayer : public UIScreen {
public:
    InventoryLayer(
        Window& window, UIRenderer& renderer, Texture2D& uiTexture, Font& font,
        const Texture2D& blockTexture, const Texture2D& itemTexture,
        Inventory& inventory, Hotbar& hotbar, const CraftingStations& stations,
        std::function<void(const Item&)> onDropItem, std::function<void()> onClose);

private:
    void OnUpdate() override;
    void OnRender() override;
    void OnEvent(Event& event) override;

    glm::vec2 PanelOrigin() const;

    glm::vec4 SlotRect(int index) const;

    int SlotAt(const glm::vec2& point) const;

    std::vector<const Recipe*> ShownRecipes() const;

    glm::vec4 CraftBarRect() const;
    glm::vec4 SelectionSlotRect() const;
    glm::vec4 RecipeRect(int displayIndex) const;
    int RecipeAt(const glm::vec2& point) const;
    float TargetScroll() const;

    float RecipeFade(const glm::vec4& recipeRect) const;

    void Close();

    std::function<void(const Item&)> m_OnDropItem;
    std::function<void()> m_OnClose;

    ItemSlots m_Slots;
    CraftingStations m_Stations;

    const Texture2D& m_BlockTexture;
    const Texture2D& m_ItemTexture;

    Item m_Held;

    int m_SelectedRecipe = 0;
    float m_CraftScroll = 0.0f;
    bool m_CraftScrollReady = false;

    const Recipe* m_HeldRecipe = nullptr;
    float m_NextCraftTime = 0.0f;
};

}
