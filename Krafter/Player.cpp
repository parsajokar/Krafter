#include "imgui.h"

#include "Krafter/Core/Event.h"
#include "Krafter/Core/Window.h"
#include "Krafter/Hotbar.h"
#include "Krafter/Player.h"
#include "Krafter/World/World.h"

namespace Krafter {

namespace {

const char* BlockName(Block block)
{
    switch (block) {
    case Block::k_Air: return "Air";
    case Block::k_Dirt: return "Dirt";
    case Block::k_Grass: return "Grass";
    case Block::k_Sand: return "Sand";
    case Block::k_Water: return "Water";
    case Block::k_OakLog: return "Oak Log";
    case Block::k_OakLeaves: return "Oak Leaves";
    case Block::k_BirchLog: return "Birch Log";
    case Block::k_BirchLeaves: return "Birch Leaves";
    case Block::k_AcaciaLog: return "Acacia Log";
    case Block::k_AcaciaLeaves: return "Acacia Leaves";
    case Block::k_OakWood: return "Oak Wood";
    case Block::k_BirchWood: return "Birch Wood";
    case Block::k_AcaciaWood: return "Acacia Wood";
    case Block::k_ShortGrass: return "Short Grass";
    case Block::k_Fern: return "Fern";
    case Block::k_DeadBush: return "Dead Bush";
    case Block::k_Cactus: return "Cactus";
    case Block::k_OakPlanks: return "Oak Planks";
    case Block::k_BirchPlanks: return "Birch Planks";
    case Block::k_AcaciaPlanks: return "Acacia Planks";
    case Block::k_Stone: return "Stone";
    case Block::k_Bedrock: return "Bedrock";
    case Block::k_Lava: return "Lava";
    case Block::k_Torch: return "Torch";
    case Block::k_IronOre: return "Iron Ore";
    case Block::k_CopperOre: return "Copper Ore";
    case Block::k_CoalOre: return "Coal Ore";
    case Block::k_Topaz: return "Topaz";
    case Block::k_Emerald: return "Emerald";
    case Block::k_Amethyst: return "Amethyst";
    case Block::k_Diamond: return "Diamond";
    case Block::k_HardIce: return "Hard Ice";
    case Block::k_Ice: return "Ice";
    case Block::k_CactusFlower: return "Cactus Flower";
    case Block::k_Poppy: return "Poppy";
    case Block::k_Dandelion: return "Dandelion";
    case Block::k_Allium: return "Allium";
    case Block::k_RedSand: return "Red Sand";
    case Block::k_Workbench: return "Workbench";
    case Block::k_Furnace: return "Furnace";
    case Block::k_Count: break;
    }
    return "Unknown";
}

const char* ItemKindName(ItemKind kind)
{
    switch (kind) {
    case ItemKind::k_WoodenAxe: return "Wooden Axe";
    case ItemKind::k_WoodenPickaxe: return "Wooden Pickaxe";
    case ItemKind::k_WoodenShovel: return "Wooden Shovel";
    case ItemKind::k_WoodenSword: return "Wooden Sword";
    case ItemKind::k_Coal: return "Coal";
    }
    return "Unknown";
}

}

Player::Player(Window& window, World& world, const glm::vec3& position, float fov, GameMode mode)
    : m_Window(window)
    , m_World(world)
    , m_Camera(position, fov)
    , m_Mode(mode)
    , m_Speed(mode == GameMode::k_Survival ? k_DefaultWalkSpeed : k_DefaultFlySpeed)
{
    const glm::ivec2& size = m_Window.GetSize();
    m_Camera.SetViewportSize(size.x, size.y);
    ApplyControlMode();
}

void Player::Update()
{
    for (const Item& drop : m_World.TakeDrops()) {
        CollectDrop(drop);
    }

    if (m_Mode == GameMode::k_Survival) {
        if (m_PhysicsEnabled) {
            UpdateSurvival();
        }
    } else if (m_IsControlled) {
        UpdateSpectator();
    }

    glm::ivec3 hit;
    glm::ivec3 before;
    m_HasTarget = RaycastTarget(hit, before);
    if (m_HasTarget) {
        m_TargetBlock = hit;
    }

    UpdateBreaking();

    if (m_IsControlled && m_PlaceHeld) {
        m_PlaceCooldown -= m_Window.GetDelta();
        if (m_PlaceCooldown <= 0.0f) {
            PlaceTargetBlock();
            m_PlaceCooldown = m_PlaceInterval;
        }
    }
}

void Player::UpdateSpectator()
{
    float delta = m_Window.GetDelta();

    glm::vec3 direction = m_Camera.GetDirection();
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::normalize(glm::cross(direction, up));

    glm::vec3 position = m_Camera.GetPosition()
        + (right * m_MoveInput.x + direction * m_MoveInput.y) * m_Speed * delta;
    m_Camera.SetPosition(position);
}

void Player::UpdateSurvival()
{
    glm::vec3 position = m_Camera.GetPosition();

    if (!m_World.IsChunkLoaded(position)) {
        m_VerticalVelocity = 0.0f;
        return;
    }

    const float delta = m_Window.GetDelta();

    glm::vec3 direction = m_Camera.GetDirection();
    glm::vec3 forward = glm::vec3(direction.x, 0.0f, direction.z);
    if (glm::dot(forward, forward) > 0.0f) {
        forward = glm::normalize(forward);
    }
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));

    glm::vec3 wish = right * m_MoveInput.x + forward * m_MoveInput.y;
    if (glm::dot(wish, wish) > 1.0f) {
        wish = glm::normalize(wish);
    }
    const glm::vec3 horizontal = wish * m_Speed;

    if (m_JumpHeld && m_OnGround) {
        m_VerticalVelocity = m_JumpSpeed;
        m_OnGround = false;
    }

    m_VerticalVelocity = glm::max(m_VerticalVelocity - m_Gravity * delta, -m_TerminalSpeed);

    position.x += horizontal.x * delta;
    if (CollidesAt(position)) {
        position.x = m_Camera.GetPosition().x;
    }

    position.z += horizontal.z * delta;
    if (CollidesAt(position)) {
        position.z = m_Camera.GetPosition().z;
    }

    m_OnGround = false;
    position.y += m_VerticalVelocity * delta;
    if (CollidesAt(position)) {
        position.y = m_Camera.GetPosition().y;
        m_OnGround = m_VerticalVelocity < 0.0f;
        m_VerticalVelocity = 0.0f;
    }

    m_Camera.SetPosition(position);
}

void Player::BodyCellBounds(const glm::vec3& eye, glm::ivec3& outLo, glm::ivec3& outHi) const
{
    const float half = m_Width * 0.5f;
    const glm::vec3 min = eye - glm::vec3(half, m_EyeHeight, half);
    const glm::vec3 max = eye + glm::vec3(half, m_Height - m_EyeHeight, half);
    outLo = glm::ivec3(glm::floor(min));
    outHi = glm::ivec3(glm::floor(max));
}

bool Player::CollidesAt(const glm::vec3& eye) const
{
    glm::ivec3 lo;
    glm::ivec3 hi;
    BodyCellBounds(eye, lo, hi);

    for (int32_t x = lo.x; x <= hi.x; x++) {
        for (int32_t y = lo.y; y <= hi.y; y++) {
            for (int32_t z = lo.z; z <= hi.z; z++) {
                if (IsOpaque(m_World.GetBlock(glm::ivec3(x, y, z)))) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool Player::OccupiesCell(const glm::ivec3& cell) const
{
    glm::ivec3 lo;
    glm::ivec3 hi;
    BodyCellBounds(m_Camera.GetPosition(), lo, hi);

    return cell.x >= lo.x && cell.x <= hi.x
        && cell.y >= lo.y && cell.y <= hi.y
        && cell.z >= lo.z && cell.z <= hi.z;
}

bool Player::RaycastTarget(glm::ivec3& hit, glm::ivec3& before) const
{
    return m_World.RaycastBlock(m_Camera.GetPosition(), m_Camera.GetDirection(), m_Reach, hit, before);
}

void Player::UpdateBreaking()
{
    if (!m_IsControlled || !m_BreakHeld || !m_HasTarget
        || !CanBreakWith(m_Hotbar.GetSelectedItem(), m_World.GetBlock(m_TargetBlock))) {
        m_IsBreaking = false;
        m_BreakProgress = 0.0f;
        return;
    }

    if (!m_IsBreaking || m_TargetBlock != m_BreakBlock) {
        m_BreakBlock = m_TargetBlock;
        m_BreakProgress = 0.0f;
        m_IsBreaking = true;
    }

    m_BreakProgress += m_Window.GetDelta();

    if (m_BreakProgress >= BreakSeconds(m_World.GetBlock(m_BreakBlock))) {
        const Block broken = m_World.GetBlock(m_BreakBlock);
        m_World.SetBlock(m_BreakBlock, Block::k_Air);
        m_World.SpawnDrop(glm::vec3(m_BreakBlock) + 0.5f, DropItemFor(broken));
        m_IsBreaking = false;
        m_BreakProgress = 0.0f;
    }
}

void Player::CollectDrop(const Item& drop)
{
    if (drop.IsEmpty()) {
        return;
    }

    for (int slot = 0; slot < Hotbar::k_SlotCount; ++slot) {
        Item item = m_Hotbar.GetItem(slot);
        if (item == drop && item.count < Item::k_MaxStack) {
            item.count += drop.count;
            m_Hotbar.SetItem(slot, item);
            return;
        }
    }
    for (int slot = 0; slot < Inventory::k_SlotCount; ++slot) {
        Item item = m_Inventory.GetItem(slot);
        if (item == drop && item.count < Item::k_MaxStack) {
            item.count += drop.count;
            m_Inventory.SetItem(slot, item);
            return;
        }
    }

    for (int slot = 0; slot < Hotbar::k_SlotCount; ++slot) {
        if (m_Hotbar.GetItem(slot).IsEmpty()) {
            m_Hotbar.SetItem(slot, drop);
            return;
        }
    }
    for (int slot = 0; slot < Inventory::k_SlotCount; ++slot) {
        if (m_Inventory.GetItem(slot).IsEmpty()) {
            m_Inventory.SetItem(slot, drop);
            return;
        }
    }
}

float Player::GetBreakProgress() const
{
    const float breakTime = BreakSeconds(m_World.GetBlock(m_BreakBlock));
    if (breakTime <= 0.0f) {
        return 1.0f;
    }
    return glm::clamp(m_BreakProgress / breakTime, 0.0f, 1.0f);
}

void Player::PlaceTargetBlock()
{
    glm::ivec3 hit;
    glm::ivec3 before;
    if (!RaycastTarget(hit, before)) {
        return;
    }

    const Item selected = m_Hotbar.GetSelectedItem();
    if (!selected.IsBlock()) {
        return;
    }
    const Block block = selected.block;

    const glm::ivec3 target = IsPlant(m_World.GetBlock(hit)) ? hit : before;

    if (m_Mode == GameMode::k_Survival && OccupiesCell(target)) {
        return;
    }

    m_World.PlaceBlock(target, block);

    Item held = selected;
    held.count--;
    m_Hotbar.SetItem(m_Hotbar.GetSelected(), held.count > 0 ? held : Item());
}

void Player::OnEvent(Event& event)
{
    if (event.button == MouseButton::k_Left) {
        if (event.type == EventType::k_MouseButtonPressed) {
            m_BreakHeld = true;
            event.handled = true;
            return;
        }
        if (event.type == EventType::k_MouseButtonReleased) {
            m_BreakHeld = false;
            event.handled = true;
            return;
        }
    }

    if (event.button == MouseButton::k_Right) {
        if (event.type == EventType::k_MouseButtonPressed) {
            m_PlaceHeld = true;
            m_PlaceCooldown = m_PlaceInterval;
            PlaceTargetBlock();
            event.handled = true;
            return;
        }
        if (event.type == EventType::k_MouseButtonReleased) {
            m_PlaceHeld = false;
            event.handled = true;
            return;
        }
    }

    auto applyMove = [&](float sign) {
        switch (event.key) {
        case Key::k_W:
            m_MoveInput.y += sign;
            break;
        case Key::k_S:
            m_MoveInput.y -= sign;
            break;
        case Key::k_D:
            m_MoveInput.x += sign;
            break;
        case Key::k_A:
            m_MoveInput.x -= sign;
            break;
        default:
            break;
        }
    };

    switch (event.type) {
    case EventType::k_KeyPressed:
        if (event.key == Key::k_Space) {
            if (m_Mode == GameMode::k_Survival) {
                m_JumpHeld = true;
                event.handled = true;
            }
        } else if (!event.isRepeat) {
            applyMove(1.0f);
        }
        break;

    case EventType::k_KeyReleased:
        if (event.key == Key::k_Space) {
            m_JumpHeld = false;
        } else {
            applyMove(-1.0f);
        }
        break;

    case EventType::k_MouseMoved: {
        if (!m_IsControlled) {
            break;
        }
        if (m_FirstMouse) {
            m_LastCursorPosition = event.mouse;
            m_FirstMouse = false;
            break;
        }

        glm::vec2 offset = event.mouse - m_LastCursorPosition;
        m_LastCursorPosition = event.mouse;

        float yaw = m_Camera.GetYaw() + offset.x * m_Sensitivity / 5000.0f;
        float pitch = m_Camera.GetPitch() - offset.y * m_Sensitivity / 5000.0f;

        pitch = glm::clamp(pitch, glm::radians(-89.99f), glm::radians(89.99f));
        if (yaw < 0.0f) {
            yaw += glm::radians(360.0f);
        } else if (yaw > glm::radians(360.0f)) {
            yaw -= glm::radians(360.0f);
        }
        m_Camera.SetRotation(yaw, pitch);
        break;
    }

    case EventType::k_WindowResized:
        m_Camera.SetViewportSize(event.size.x, event.size.y);
        break;

    default:
        break;
    }
}

void Player::RenderImGui()
{
    ImGui::Text("Player");
    ImGui::SliderFloat("Movement Speed", &m_Speed, 1.0f, 100.0f);
    ImGui::SliderFloat("Mouse Sensitivity", &m_Sensitivity, 1.0f, 100.0f);

    float fov = glm::degrees(m_Camera.GetFieldOfView());
    if (ImGui::SliderFloat("Field of View", &fov, 30.0f, 110.0f)) {
        m_Camera.SetFieldOfView(glm::radians(fov));
    }
    ImGui::SliderFloat("Reach", &m_Reach, 1.0f, 32.0f);
    ImGui::SliderFloat("Place Interval", &m_PlaceInterval, 0.0f, 1.0f, "%.2fs");

    if (ImGui::TreeNode("Physics")) {
        ImGui::SliderFloat("Gravity", &m_Gravity, 0.0f, 100.0f);
        ImGui::SliderFloat("Jump Speed", &m_JumpSpeed, 0.0f, 30.0f);
        ImGui::SliderFloat("Terminal Speed", &m_TerminalSpeed, 1.0f, 200.0f);
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Collision Box")) {
        ImGui::SliderFloat("Width", &m_Width, 0.1f, 2.0f);
        ImGui::SliderFloat("Height", &m_Height, 0.5f, 4.0f);
        ImGui::SliderFloat("Eye Height", &m_EyeHeight, 0.1f, m_Height);
        ImGui::TreePop();
    }

    ImGui::Text("Yaw: %.2f, Pitch: %.2f", glm::degrees(m_Camera.GetYaw()), glm::degrees(m_Camera.GetPitch()));

    const glm::vec3 position = m_Camera.GetPosition();
    ImGui::Text("Position: %.2f, %.2f, %.2f", position.x, position.y, position.z);

    if (ImGui::TreeNode("Give Item")) {
        static int quantity = 1;
        ImGui::SliderInt("Quantity", &quantity, 1, Item::k_MaxStack);

        ImGui::BeginChild("give_list", ImVec2(0.0f, 220.0f), true);
        for (int b = 1; b < static_cast<int>(Block::k_Count); ++b) {
            const Block block = static_cast<Block>(b);
            if (ImGui::Selectable(BlockName(block))) {
                Item item(block);
                item.count = quantity;
                CollectDrop(item);
            }
        }
        for (const ItemKind kind :
            { ItemKind::k_WoodenAxe, ItemKind::k_WoodenPickaxe, ItemKind::k_WoodenShovel,
                ItemKind::k_WoodenSword, ItemKind::k_Coal }) {
            if (ImGui::Selectable(ItemKindName(kind))) {
                CollectDrop(Item::Material(kind, quantity));
            }
        }
        ImGui::EndChild();
        ImGui::TreePop();
    }
}

void Player::SetControlled(bool controlled)
{
    m_IsControlled = controlled;
    m_PhysicsEnabled = controlled;
    ApplyControlMode();

    m_MoveInput = glm::vec2(0.0f);
    m_VerticalVelocity = 0.0f;
    m_JumpHeld = false;
    m_PlaceHeld = false;
    m_BreakHeld = false;
    m_IsBreaking = false;
    m_BreakProgress = 0.0f;
    m_FirstMouse = true;
}

void Player::SuspendForInventory()
{
    m_IsControlled = false;
    ApplyControlMode();

    m_MoveInput = glm::vec2(0.0f);
    m_JumpHeld = false;
    m_PlaceHeld = false;
    m_BreakHeld = false;
    m_IsBreaking = false;
    m_BreakProgress = 0.0f;
    m_FirstMouse = true;
}

void Player::ApplyControlMode()
{
    m_Window.SetCursor(!m_IsControlled);

    ImGuiIO& io = ImGui::GetIO();
    if (m_IsControlled) {
        io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
    } else {
        io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
    }
}

}
