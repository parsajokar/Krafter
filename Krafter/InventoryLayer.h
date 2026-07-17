#pragma once

#include <functional>
#include <vector>

#include "glm/glm.hpp"

#include "Krafter/Item.h"
#include "Krafter/Renderer/Texture.h"
#include "Krafter/UIScreen.h"

namespace Krafter {

class Window;
class UIRenderer;
class Font;
class Inventory;
class Hotbar;
struct Recipe;
struct Ingredient;

class InventoryLayer : public UIScreen {
public:
    InventoryLayer(
        Window& window, UIRenderer& renderer, Texture2D& uiTexture, Font& font,
        Inventory& inventory, Hotbar& hotbar, bool nearWorkbench, std::function<void()> onClose);

private:
    void OnUpdate() override;
    void OnRender() override;
    void OnEvent(Event& event) override;

    glm::vec2 PanelOrigin() const;

    glm::vec4 SlotRect(int index) const;

    int SlotAt(const glm::vec2& point) const;

    Item GetSlot(int index) const;
    void SetSlot(int index, Item item);

    void ClickSlot(int index);

    std::vector<const Recipe*> AvailableRecipes() const;

    glm::vec4 CraftBarRect() const;
    glm::vec4 SelectionSlotRect() const;
    glm::vec4 RecipeRect(int displayIndex) const;
    int RecipeAt(const glm::vec2& point) const;
    float TargetScroll() const;

    float RecipeFade(const glm::vec4& recipeRect) const;

    int CountIngredient(const Ingredient& ingredient) const;
    void ConsumeIngredient(const Ingredient& ingredient, int count);

    bool CanCraft(const Recipe& recipe) const;
    void Craft(const Recipe& recipe);

    void Close();

    std::function<void()> m_OnClose;

    Inventory& m_Inventory;
    Hotbar& m_Hotbar;

    bool m_NearWorkbench;

    Texture2D m_BlockTexture;
    Texture2D m_ItemTexture;

    Item m_Held;

    int m_SelectedRecipe = 0;
    float m_CraftScroll = 0.0f;
    bool m_CraftScrollReady = false;

    const Recipe* m_HeldRecipe = nullptr;
    float m_NextCraftTime = 0.0f;
};

}
