#include "imgui.h"

#include "Krafter/Core/Event.h"
#include "Krafter/Core/Window.h"
#include "Krafter/Hotbar.h"
#include "Krafter/Player.h"
#include "Krafter/World/World.h"

namespace Krafter {

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
    // Stow whatever the world dropped as blocks fell away (a chopped tree's
    // stranded logs, a toppled cactus); the directly mined block is stowed in
    // UpdateBreaking when its break completes.
    for (const Block drop : m_World.TakeDrops()) {
        CollectDrop(drop);
    }

    if (m_Mode == GameMode::k_Survival) {
        // Survival physics run whenever enabled, even when the player isn't in
        // control (the inventory screen): movement input is zero while
        // uncontrolled, so only gravity and existing velocity move the body, and
        // a fall already underway keeps its momentum.
        if (m_PhysicsEnabled) {
            UpdateSurvival();
        }
    } else if (m_IsControlled) {
        // Spectator flight is pure input with no momentum, so it only moves while
        // the player is in control.
        UpdateSpectator();
    }

    glm::ivec3 hit;
    glm::ivec3 before;
    m_HasTarget = RaycastTarget(hit, before);
    if (m_HasTarget) {
        m_TargetBlock = hit;
    }

    UpdateBreaking();

    // Holding right click keeps placing on a fixed cadence; the first placement
    // fired on the press, so this only handles the repeats.
    if (m_IsControlled && m_PlaceHeld) {
        m_PlaceCooldown -= m_Window.GetDelta();
        if (m_PlaceCooldown <= 0.0f) {
            PlaceTargetBlock();
            m_PlaceCooldown = k_PlaceInterval;
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

    // Hold the player in place until the terrain below has generated, so it
    // doesn't fall through the not-yet-loaded world on first spawn.
    if (!m_World.IsChunkLoaded(position)) {
        m_VerticalVelocity = 0.0f;
        return;
    }

    const float delta = m_Window.GetDelta();

    // Walk along the ground plane: the look direction flattened to XZ, so looking
    // up or down never lifts or sinks the player.
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

    // Jump while grounded; holding the key re-jumps on each landing.
    if (m_JumpHeld && m_OnGround) {
        m_VerticalVelocity = k_JumpSpeed;
        m_OnGround = false;
    }

    // Gravity, capped at a terminal speed so a long fall can't tunnel through the
    // ground in a single step.
    m_VerticalVelocity = glm::max(m_VerticalVelocity - k_Gravity * delta, -k_TerminalSpeed);

    // Resolve each axis on its own so a wall stops only that axis and the player
    // slides along it instead of sticking.
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
        // Hitting something while falling means we've landed; while rising it's a
        // ceiling. Either way the vertical motion stops.
        m_OnGround = m_VerticalVelocity < 0.0f;
        m_VerticalVelocity = 0.0f;
    }

    m_Camera.SetPosition(position);
}

void Player::BodyCellBounds(const glm::vec3& eye, glm::ivec3& outLo, glm::ivec3& outHi) const
{
    const float half = k_Width * 0.5f;
    const glm::vec3 min = eye - glm::vec3(half, k_EyeHeight, half);
    const glm::vec3 max = eye + glm::vec3(half, k_Height - k_EyeHeight, half);
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
    return m_World.RaycastBlock(m_Camera.GetPosition(), m_Camera.GetDirection(), k_Reach, hit, before);
}

void Player::UpdateBreaking()
{
    // The break only runs while the button is held, in control, aimed at a block
    // the tool can break. Otherwise it lapses and the crack clears.
    if (!m_IsControlled || !m_BreakHeld || !m_HasTarget
        || !CanBreakWith(m_Hotbar.GetSelectedItem(), m_World.GetBlock(m_TargetBlock))) {
        m_IsBreaking = false;
        m_BreakProgress = 0.0f;
        return;
    }

    // Re-aiming at a different block restarts the break from the first crack.
    if (!m_IsBreaking || m_TargetBlock != m_BreakBlock) {
        m_BreakBlock = m_TargetBlock;
        m_BreakProgress = 0.0f;
        m_IsBreaking = true;
    }

    m_BreakProgress += m_Window.GetDelta();

    // Reaching the block's break time removes it. The break then lapses so the
    // next held frame starts fresh on whatever the player is now aimed at,
    // letting a held swing chew through a row of blocks one after another.
    if (m_BreakProgress >= BreakSeconds(m_World.GetBlock(m_BreakBlock))) {
        const Block broken = m_World.GetBlock(m_BreakBlock);
        m_World.SetBlock(m_BreakBlock, Block::k_Air);
        // Drop the block into the world to be walked over, rather than stowing it
        // straight away; the cascade of a felled tree drops its logs the same way.
        m_World.SpawnDrop(glm::vec3(m_BreakBlock) + 0.5f, DropFor(broken));
        m_IsBreaking = false;
        m_BreakProgress = 0.0f;
    }
}

void Player::CollectDrop(Block drop)
{
    if (drop == Block::k_Air) {
        return;
    }

    // First top up an existing stack of the same block that still has room, so a
    // run of logs piles into one slot instead of scattering across many. Scan the
    // hotbar before the grid so quick-select stacks fill first.
    for (int slot = 0; slot < Hotbar::k_SlotCount; ++slot) {
        Item item = m_Hotbar.GetItem(slot);
        if (item.IsBlock() && item.block == drop && item.count < Item::k_MaxStack) {
            item.count++;
            m_Hotbar.SetItem(slot, item);
            return;
        }
    }
    for (int slot = 0; slot < Inventory::k_SlotCount; ++slot) {
        Item item = m_Inventory.GetItem(slot);
        if (item.IsBlock() && item.block == drop && item.count < Item::k_MaxStack) {
            item.count++;
            m_Inventory.SetItem(slot, item);
            return;
        }
    }

    // No partial stack to join; start a fresh one in the first empty slot (hotbar
    // first, so a felled log is ready to place), then the inventory grid.
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
        return; // an empty slot or a tool (such as the axe): nothing to place
    }
    const Block block = selected.block;

    // Replaceable foliage is overwritten in place; everything else is placed
    // against the targeted face.
    const glm::ivec3 target = IsPlant(m_World.GetBlock(hit)) ? hit : before;

    // In survival the body collides, so refuse a placement that would land inside
    // the player and seal them in.
    if (m_Mode == GameMode::k_Survival && OccupiesCell(target)) {
        return;
    }

    m_World.PlaceBlock(target, block);

    // Consume one from the held stack; emptying the slot clears it back to k_Air.
    Item held = selected;
    held.count--;
    m_Hotbar.SetItem(m_Hotbar.GetSelected(), held.count > 0 ? held : Item());
}

void Player::OnEvent(Event& event)
{
    if (event.button == MouseButton::k_Left) {
        // Breaking is gradual now: holding the button mines the aimed-at block
        // over its break time (Update drives the progress and the actual
        // removal), so the press and release only track whether it is held.
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
            // Place once now, then keep placing while the button stays held.
            m_PlaceHeld = true;
            m_PlaceCooldown = k_PlaceInterval;
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

    // Opposite keys cancel; auto-repeats are ignored so a held key counts once.
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
            // Survival jumps (hold to keep jumping on each landing); spectator
            // flies freely and leaves space unbound.
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
    ImGui::SliderFloat("Movement Speed", &m_Speed, 1.0f, 100.0f);
    ImGui::SliderFloat("Mouse Sensitivity", &m_Sensitivity, 1.0f, 100.0f);

    float fov = glm::degrees(m_Camera.GetFieldOfView());
    if (ImGui::SliderFloat("Field of View", &fov, 30.0f, 110.0f)) {
        m_Camera.SetFieldOfView(glm::radians(fov));
    }
    ImGui::Text("Yaw: %.2f, Pitch: %.2f", glm::degrees(m_Camera.GetYaw()), glm::degrees(m_Camera.GetPitch()));

    const glm::vec3 position = m_Camera.GetPosition();
    ImGui::Text("Position: %.2f, %.2f, %.2f", position.x, position.y, position.z);
}

void Player::SetControlled(bool controlled)
{
    m_IsControlled = controlled;
    // Taking control resumes physics; releasing it for a menu freezes the player
    // in place. (The inventory uses SuspendForInventory, which keeps physics on.)
    m_PhysicsEnabled = controlled;
    ApplyControlMode();

    // Reset movement and rebaseline the look so toggling doesn't drift or snap.
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
    // Release look and input and free the cursor for the inventory screen, but
    // leave m_PhysicsEnabled and m_VerticalVelocity untouched so the survival
    // simulation keeps integrating and a fall in progress carries its momentum.
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

} // namespace Krafter
