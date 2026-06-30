#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include "imgui.h"

#include "Krafter/Renderer/WorldRenderer.h"
#include "Krafter/World/Biome.h"
#include "Krafter/World/Coords.h"
#include "Krafter/World/Lighting.h"
#include "Krafter/World/Sky.h"
#include "Krafter/World/World.h"

namespace Krafter {

World::World(int32_t seed)
{
    // Configure generation before the job system starts producing chunks.
    Biome::Configure(seed);
    Chunk::SetSeed(static_cast<uint32_t>(seed));
}

// m_JobSystem is declared last, so it is destroyed first: its workers stop and
// join before the result queues they push into are torn down.
World::~World() = default;

void World::Update(const glm::vec3& cameraPosition, float deltaTime)
{
    DrainResults();
    UpdateFallingBlocks(deltaTime);

    m_Time += deltaTime;
    m_LastCameraPosition = cameraPosition;
    UpdateDrops(deltaTime, cameraPosition);

    glm::ivec2 origin = glm::ivec2(cameraPosition.x, cameraPosition.z) / Chunk::k_Width;

    for (int32_t x = -m_RenderDistance - 2; x <= m_RenderDistance + 2; x++) {
        for (int32_t z = -m_RenderDistance - 2; z <= m_RenderDistance + 2; z++) {
            glm::ivec2 position = origin + glm::ivec2(x, z);
            if (!IsInRadius(position, origin, m_RenderDistance + 2)) {
                continue;
            }
            if (m_Chunks.count(position) || m_PendingTerrain.count(position)) {
                continue;
            }
            m_PendingTerrain.insert(position);
            m_JobSystem.Dispatch([this, position] {
                auto chunk = std::make_shared<Chunk>(position);
                m_TerrainResults.Push({ position, std::move(chunk) });
            });
        }
    }

    // Lighting stage: compute a chunk's sky light from its full 3x3 block
    // context and write it into the chunk's storage.
    for (int32_t x = -m_RenderDistance - 1; x <= m_RenderDistance + 1; x++) {
        for (int32_t z = -m_RenderDistance - 1; z <= m_RenderDistance + 1; z++) {
            glm::ivec2 position = origin + glm::ivec2(x, z);
            if (!IsInRadius(position, origin, m_RenderDistance + 1)) {
                continue;
            }
            auto it = m_Chunks.find(position);
            if (it == m_Chunks.end() || it->second.state != ChunkState::k_TerrainReady) {
                continue;
            }
            if (m_PendingLight.count(position) || !HasAllTerrainNeighbours(position)) {
                continue;
            }

            m_PendingLight.insert(position);
            m_JobSystem.Dispatch([this, position, grid = Gather3x3(position)] {
                ComputeSkyLight(*grid[4], AsPointers(grid));
                m_LightResults.Push(position);
            });
        }
    }

    // Meshing stage: only once the chunk and all 8 neighbours are lit, so
    // border smooth-lighting and AO sample settled, seam-free light.
    for (int32_t x = -m_RenderDistance; x <= m_RenderDistance; x++) {
        for (int32_t z = -m_RenderDistance; z <= m_RenderDistance; z++) {
            glm::ivec2 position = origin + glm::ivec2(x, z);
            if (!IsInRadius(position, origin, m_RenderDistance)) {
                continue;
            }
            auto it = m_Chunks.find(position);
            if (it == m_Chunks.end() || it->second.state != ChunkState::k_LightReady) {
                continue;
            }
            if (m_PendingMesh.count(position) || !HasAllLitNeighbours(position)) {
                continue;
            }

            m_PendingMesh.insert(position);
            m_JobSystem.Dispatch([this, position, grid = Gather3x3(position)] {
                ChunkMeshData data = ChunkMesh::Compute(AsPointers(grid), position);
                m_MeshResults.Push({ position, std::move(data) });
            });
        }
    }

    for (auto it = m_Chunks.begin(); it != m_Chunks.end();) {
        if (!IsInRadius(it->first, origin, m_RenderDistance + 2)) {
            it = m_Chunks.erase(it);
        } else {
            ++it;
        }
    }
}

void World::Render(WorldRenderer& renderer, const glm::mat4& viewProjection, const Sky& sky)
{
    // Opaque geometry first, writing depth as usual. Back-face culling hides
    // block interiors that would otherwise show through cutout foliage.
    renderer.SetCullFace(true);
    for (const auto& [position, record] : m_Chunks) {
        if (record.mesh) {
            renderer.RenderChunkOpaque(*record.mesh, viewProjection, sky);
        }
    }
    renderer.SetCullFace(false);

    // Floating item drops: opaque, depth-writing billboards that always face the
    // camera, so each reads as its HUD icon hanging in the world. A gentle bob
    // animates them; the basis is rebuilt per drop from the eye recorded in Update.
    constexpr glm::vec3 k_WorldUp(0.0f, 1.0f, 0.0f);
    for (const ItemDrop& drop : m_Drops) {
        glm::vec3 center = drop.position;
        center.y += std::sin((m_Time + drop.phase) * k_DropBobSpeed) * k_DropBobAmplitude;

        const glm::vec3 toEye = m_LastCameraPosition - center;
        if (glm::dot(toEye, toEye) < 1e-6f) {
            continue; // degenerate: the eye is right on the drop
        }
        const glm::vec3 forward = glm::normalize(toEye);
        const glm::vec3 right = glm::normalize(glm::cross(k_WorldUp, forward));
        const glm::vec3 up = glm::cross(forward, right);

        renderer.RenderItemDrop(
            center, right * k_DropSize, up * k_DropSize, BlockIconTile(drop.block), viewProjection);
    }

    // Cross plants next: cutout (the shader discards clear texels) and still
    // depth-writing like opaque geometry, but drawn double-sided so both faces of
    // each billboard show, so culling stays off.
    for (const auto& [position, record] : m_Chunks) {
        if (record.mesh) {
            renderer.RenderChunkCross(*record.mesh, viewProjection, sky);
        }
    }

    // Then water: it blends over what is already there and must not occlude
    // other water behind it, so blending is on and depth writes are disabled.
    renderer.SetBlend(true);
    renderer.SetDepthMask(false);
    for (const auto& [position, record] : m_Chunks) {
        if (record.mesh) {
            renderer.RenderChunkTransparent(*record.mesh, viewProjection, sky);
        }
    }
    renderer.SetDepthMask(true);
    renderer.SetBlend(false);
}

void World::RenderImGui()
{
    ImGui::InputInt("Render Distance", &m_RenderDistance);
    ImGui::InputInt("Max Mesh Uploads Per Frame", &m_MaxMeshUploadsPerFrame);
    ImGui::Text("Workers: %zu", m_JobSystem.WorkerCount());
    ImGui::Text("Chunks: %zu", m_Chunks.size());
}

Block World::GetBlock(const glm::ivec3& worldPosition) const
{
    if (worldPosition.y < 0 || worldPosition.y >= Chunk::k_Height) {
        return Block::k_Air;
    }

    auto it = m_Chunks.find(ToChunkPosition(worldPosition));
    if (it == m_Chunks.end() || !it->second.chunk) {
        return Block::k_Air;
    }
    return it->second.chunk->GetBlock(ToLocalPosition(worldPosition));
}

bool World::IsChunkLoaded(const glm::vec3& worldPosition) const
{
    const glm::ivec2 chunkPosition = ToChunkPosition(glm::ivec3(glm::floor(worldPosition)));
    auto it = m_Chunks.find(chunkPosition);
    return it != m_Chunks.end() && it->second.chunk != nullptr;
}

void World::SetBlock(const glm::ivec3& worldPosition, Block block)
{
    if (worldPosition.y < 0 || worldPosition.y >= Chunk::k_Height) {
        return;
    }

    glm::ivec2 chunkPosition = ToChunkPosition(worldPosition);
    auto it = m_Chunks.find(chunkPosition);
    if (it == m_Chunks.end() || !it->second.chunk || !CanEdit(chunkPosition)) {
        return;
    }

    Chunk& chunk = *it->second.chunk;
    const Block previous = chunk.GetBlock(ToLocalPosition(worldPosition));
    chunk.SetBlock(ToLocalPosition(worldPosition), block);

    // Plants and cactus need solid ground under them. When the block we changed
    // stops being that ground, break the fragile column above it: each block
    // that falls in turn removes the support of the one above. The whole column
    // is the same (x, z), so it never leaves this chunk. Cactus segments topple
    // gradually, so they are scheduled; grass and ferns just wink out.
    std::vector<glm::ivec3> topplingCactus;
    for (int32_t y = worldPosition.y + 1; y < Chunk::k_Height; y++) {
        const glm::ivec3 cell(worldPosition.x, y, worldPosition.z);
        const glm::ivec3 local = ToLocalPosition(cell);
        const Block fragile = chunk.GetBlock(local);
        if (!IsPlant(fragile) && fragile != Block::k_Cactus) {
            break;
        }
        // The block below holds this one up unless it is air, already falling,
        // or part of the column we are toppling right now (it sits one cell down,
        // so it is the last cactus we pushed).
        const glm::ivec3 below = cell - glm::ivec3(0, 1, 0);
        const bool belowToppling = m_Falling.count(below)
            || (!topplingCactus.empty() && topplingCactus.back() == below);
        if (IsOpaque(chunk.GetBlock(ToLocalPosition(below))) && !belowToppling) {
            break; // still supported, and so is anything stacked on it
        }
        if (fragile == Block::k_Cactus) {
            topplingCactus.push_back(cell);
        } else {
            chunk.SetBlock(local, Block::k_Air);
        }
    }
    ScheduleFall(topplingCactus, worldPosition);

    // Putting a solid block beside a cactus snaps it off: the touched segment and
    // everything stacked above it break, so a cactus never ends up flush against
    // another block. A neighbour cactus can sit in an adjacent chunk, but always
    // within this edit's 3x3, so it is reachable and will be re-meshed below.
    if (IsOpaque(block)) {
        for (const glm::ivec3& side : k_HorizontalNeighbors) {
            BreakCactusColumn(worldPosition + side);
        }
    }

    // Breaking part of a tree can leave the rest of it floating. Like chopping a
    // tree in Terraria, clear away every wood/leaf block that the break stranded
    // with no path back down to the ground. Only the natural wood and leaves
    // cascade; a player-placed log is a building block and stays where it is put.
    if (IsNaturalTreePart(previous) && !IsNaturalTreePart(block)) {
        ChopFloatingTree(worldPosition);
    }

    for (int32_t dz = -1; dz <= 1; dz++) {
        for (int32_t dx = -1; dx <= 1; dx++) {
            InvalidateChunk(chunkPosition + glm::ivec2(dx, dz));
        }
    }
}

bool World::CanEdit(const glm::ivec2& chunkPosition) const
{
    // Only edit settled terrain. A block on a chunk border changes its
    // neighbours' light and faces too, so the whole 3x3 must be meshed and have
    // no light/mesh job in flight; otherwise a worker could be reading a chunk
    // we are about to mutate or invalidate.
    for (int32_t dz = -1; dz <= 1; dz++) {
        for (int32_t dx = -1; dx <= 1; dx++) {
            const glm::ivec2 neighbour = chunkPosition + glm::ivec2(dx, dz);
            auto it = m_Chunks.find(neighbour);
            if (it == m_Chunks.end() || it->second.state != ChunkState::k_MeshReady) {
                return false;
            }
            if (m_PendingLight.count(neighbour) || m_PendingMesh.count(neighbour)) {
                return false;
            }
        }
    }
    return true;
}

void World::ChopFloatingTree(const glm::ivec3& brokenPosition)
{
    // Tree blocks count as joined when they touch on any face, edge, or corner
    // (leaves hang off the trunk diagonally), so connectivity uses the full 26
    // neighbours and ground support uses the 9 cells in the layer below.
    auto forEachNeighbour = [](const glm::ivec3& cell, auto&& visit) {
        for (int32_t dy = -1; dy <= 1; dy++) {
            for (int32_t dz = -1; dz <= 1; dz++) {
                for (int32_t dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0 && dz == 0) {
                        continue;
                    }
                    visit(cell + glm::ivec3(dx, dy, dz));
                }
            }
        }
    };

    // A tree block already counting down to removal is treated as gone, so the
    // chop sees through doomed wood instead of resting the rest of the tree on it.
    // Only natural wood and leaves take part; a placed log is solid ground here.
    auto isStanding = [this](const glm::ivec3& cell) {
        return IsNaturalTreePart(GetBlock(cell)) && !m_Falling.count(cell);
    };

    std::unordered_set<glm::ivec3> visited;
    std::vector<glm::ivec3> floating;

    // Each tree block neighbouring the break may belong to a clump the break just
    // stranded. Flood every connected run of wood/leaves out from those seeds.
    std::vector<glm::ivec3> seeds;
    forEachNeighbour(brokenPosition, [&](const glm::ivec3& neighbour) { seeds.push_back(neighbour); });

    for (const glm::ivec3& seed : seeds) {
        if (visited.count(seed) || !isStanding(seed)) {
            continue;
        }

        std::vector<glm::ivec3> component;
        std::vector<glm::ivec3> stack { seed };
        visited.insert(seed);
        bool grounded = false;

        while (!stack.empty()) {
            const glm::ivec3 cell = stack.back();
            stack.pop_back();
            component.push_back(cell);

            // A tree part resting on a solid, non-tree block (dirt, grass,
            // stone, a placed log...) directly or diagonally below is footed: it
            // and everything joined to it stay.
            for (int32_t dz = -1; dz <= 1 && !grounded; dz++) {
                for (int32_t dx = -1; dx <= 1 && !grounded; dx++) {
                    const Block under = GetBlock(cell + glm::ivec3(dx, -1, dz));
                    if (IsOpaque(under) && !IsNaturalTreePart(under)) {
                        grounded = true;
                    }
                }
            }

            forEachNeighbour(cell, [&](const glm::ivec3& next) {
                if (visited.count(next) || !isStanding(next)) {
                    return;
                }
                visited.insert(next);
                stack.push_back(next);
            });
        }

        if (grounded) {
            continue;
        }
        floating.insert(floating.end(), component.begin(), component.end());
    }

    ScheduleFall(floating, brokenPosition);
}

void World::ScheduleFall(const std::vector<glm::ivec3>& cells, const glm::ivec3& origin)
{
    // Time each block by its Chebyshev distance from the break and a fixed step,
    // so the break spreads at a constant speed: the cells next to the cut go
    // almost at once, and a taller tree simply takes more steps to finish.
    for (const glm::ivec3& cell : cells) {
        if (!m_Falling.insert(cell).second) {
            continue; // already counting down from an earlier break
        }
        const glm::ivec3 d = glm::abs(cell - origin);
        const int32_t dist = std::max({ d.x, d.y, d.z });
        m_FallingBlocks.push_back({ cell, static_cast<float>(dist) * k_FallStep });
    }
}

void World::UpdateFallingBlocks(float deltaTime)
{
    if (m_FallingBlocks.empty()) {
        return;
    }

    std::unordered_set<glm::ivec2> touchedChunks;

    for (size_t i = 0; i < m_FallingBlocks.size();) {
        FallingBlock& falling = m_FallingBlocks[i];
        falling.delay -= deltaTime;
        if (falling.delay > 0.0f) {
            i++;
            continue;
        }

        const glm::ivec3 cell = falling.cell;
        const glm::ivec2 cellChunk = ToChunkPosition(cell);
        auto it = m_Chunks.find(cellChunk);

        // Its timer is up, but the chunk may have unloaded, or a worker may be
        // mid-flight on it. If it merely isn't safe to edit yet, leave the entry
        // and retry next frame; if the chunk is gone, just forget it.
        const bool unloaded = it == m_Chunks.end() || !it->second.chunk;
        if (!unloaded && !CanEdit(cellChunk)) {
            i++;
            continue;
        }
        if (!unloaded) {
            // The player may have mined or built over it in the meantime; only
            // clear it if our doomed block is still there.
            const Block here = it->second.chunk->GetBlock(ToLocalPosition(cell));
            if (IsNaturalTreePart(here) || here == Block::k_Cactus) {
                it->second.chunk->SetBlock(ToLocalPosition(cell), Block::k_Air);
                touchedChunks.insert(cellChunk);

                // A felled trunk sheds its logs the same as one mined by hand:
                // spawn a floating drop at the cell centre to fall and be walked
                // over. Leaves (and anything else without a drop) leave nothing.
                SpawnDrop(glm::vec3(cell) + 0.5f, DropFor(here));
            }
        }

        m_Falling.erase(cell);
        m_FallingBlocks[i] = m_FallingBlocks.back();
        m_FallingBlocks.pop_back();
    }

    // Clearing blocks changes light and faces across their chunks, so re-light
    // and re-mesh each touched chunk together with its 3x3.
    for (const glm::ivec2& chunkPosition : touchedChunks) {
        for (int32_t dz = -1; dz <= 1; dz++) {
            for (int32_t dx = -1; dx <= 1; dx++) {
                InvalidateChunk(chunkPosition + glm::ivec2(dx, dz));
            }
        }
    }
}

void World::SpawnDrop(const glm::vec3& position, Block block)
{
    if (block == Block::k_Air) {
        return;
    }

    // Pop the drop up and out along a turning angle (the golden angle keeps a run
    // of drops from a felled tree fanning out instead of overlapping), reusing the
    // same angle as the bob phase so each drop bobs out of step with the others.
    const float angle = static_cast<float>(m_Drops.size()) * 2.39996323f;
    const glm::vec3 velocity(
        std::cos(angle) * k_DropPopOut, k_DropPopUp, std::sin(angle) * k_DropPopOut);
    m_Drops.push_back({ position, velocity, block, 0.0f, angle });
}

void World::UpdateDrops(float deltaTime, const glm::vec3& cameraPosition)
{
    for (size_t i = 0; i < m_Drops.size();) {
        ItemDrop& drop = m_Drops[i];
        drop.age += deltaTime;

        // Home to the player's feet, not the eye the camera rides at, so drops
        // converge at ground level instead of flying up to the face.
        const glm::vec3 target = cameraPosition - glm::vec3(0.0f, k_PlayerEyeHeight, 0.0f);
        const glm::vec3 toTarget = target - drop.position;
        const float distSq = glm::dot(toTarget, toTarget);

        // Within the magnet radius (and past the pop-out delay) the drop is pulled
        // to the player: collected once it reaches the feet, otherwise gravitating
        // in and accelerating as it closes. Gravity is ignored while it homes, so
        // it can rise off the ground and streak to the feet.
        if (drop.age >= k_PickupDelay && distSq <= k_AttractRadius * k_AttractRadius) {
            if (distSq <= k_PickupRadius * k_PickupRadius) {
                m_PendingDrops.push_back(drop.block);
                m_Drops[i] = m_Drops.back();
                m_Drops.pop_back();
                continue;
            }

            const float dist = std::sqrt(distSq);
            const float closeness = 1.0f - dist / k_AttractRadius; // 0 at edge, ->1 near
            const float speed = glm::mix(k_AttractMinSpeed, k_AttractMaxSpeed, closeness);
            const float step = std::min(speed * deltaTime, dist); // never overshoot the feet
            drop.position += (toTarget / dist) * step;
            drop.velocity = glm::vec3(0.0f);
            i++;
            continue;
        }

        // Out of magnet range: fall under gravity, then rest on the first solid
        // block below, clamped to sit k_DropHalfHeight above its top face. Plants
        // and water aren't solid, so a drop falls through them to real ground. The
        // probe dips just under the feet, since at rest the feet sit exactly on a
        // cell boundary (which would otherwise floor to the empty cell above).
        drop.velocity.y -= k_DropGravity * deltaTime;
        drop.position += drop.velocity * deltaTime;

        const float feetY = drop.position.y - k_DropHalfHeight;
        const glm::ivec3 ground = glm::ivec3(glm::floor(
            glm::vec3(drop.position.x, feetY - 0.05f, drop.position.z)));
        if (drop.velocity.y <= 0.0f && IsOpaque(GetBlock(ground))) {
            drop.position.y = static_cast<float>(ground.y) + 1.0f + k_DropHalfHeight;
            drop.velocity = glm::vec3(0.0f);
        }

        // Give up on a drop that has aged out or sunk into the void (e.g. fell off
        // the bottom of the world or through unloaded chunks) so they can't pile up.
        if (drop.age >= k_DropLifetime || drop.position.y < k_DropVoidY) {
            m_Drops[i] = m_Drops.back();
            m_Drops.pop_back();
            continue;
        }

        i++;
    }
}

void World::BreakCactusColumn(const glm::ivec3& worldPosition)
{
    // Break the cactus at this cell and every cactus stacked directly above it;
    // segments below the contact keep their footing and stay. They topple
    // gradually, so the column is scheduled rather than cleared outright.
    std::vector<glm::ivec3> column;
    for (int32_t y = worldPosition.y; y < Chunk::k_Height; y++) {
        const glm::ivec3 cell(worldPosition.x, y, worldPosition.z);
        if (GetBlock(cell) != Block::k_Cactus) {
            break;
        }
        column.push_back(cell);
    }
    ScheduleFall(column, worldPosition);
}

void World::PlaceBlock(const glm::ivec3& worldPosition, Block block)
{
    if (block == Block::k_Air) {
        return;
    }

    if (IsPlant(block)) {
        // Foliage only sits on solid ground and needs an empty cell to fill.
        const glm::ivec3 below(worldPosition.x, worldPosition.y - 1, worldPosition.z);
        if (below.y < 0 || !IsOpaque(GetBlock(below)) || GetBlock(worldPosition) != Block::k_Air) {
            return;
        }
    }

    if (block == Block::k_Cactus) {
        // Cactus can't touch a solid block on any of its four sides, so cacti
        // can't be placed next to each other (or flush against terrain).
        for (const glm::ivec3& side : k_HorizontalNeighbors) {
            if (IsOpaque(GetBlock(worldPosition + side))) {
                return;
            }
        }
    }

    SetBlock(worldPosition, block);
}

bool World::RaycastBlock(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, glm::ivec3& outHit, glm::ivec3& outBefore) const
{
    // Amanatides & Woo voxel traversal: walk cell by cell along the ray.
    glm::vec3 dir = glm::normalize(direction);
    glm::ivec3 block = glm::ivec3(glm::floor(origin));

    glm::ivec3 step;
    glm::vec3 tMax;
    glm::vec3 tDelta;
    for (int32_t i = 0; i < 3; i++) {
        if (dir[i] > 0.0f) {
            step[i] = 1;
            tMax[i] = (static_cast<float>(block[i] + 1) - origin[i]) / dir[i];
            tDelta[i] = 1.0f / dir[i];
        } else if (dir[i] < 0.0f) {
            step[i] = -1;
            tMax[i] = (origin[i] - static_cast<float>(block[i])) / -dir[i];
            tDelta[i] = 1.0f / -dir[i];
        } else {
            step[i] = 0;
            tMax[i] = std::numeric_limits<float>::infinity();
            tDelta[i] = std::numeric_limits<float>::infinity();
        }
    }

    float t = 0.0f;
    glm::ivec3 previous = block;
    while (t <= maxDistance) {
        // The ray passes through water and air; solids and plants are targetable.
        if (IsTargetable(GetBlock(block))) {
            outHit = block;
            outBefore = previous;
            return true;
        }

        previous = block;
        if (tMax.x < tMax.y && tMax.x < tMax.z) {
            block.x += step.x;
            t = tMax.x;
            tMax.x += tDelta.x;
        } else if (tMax.y < tMax.z) {
            block.y += step.y;
            t = tMax.y;
            tMax.y += tDelta.y;
        } else {
            block.z += step.z;
            t = tMax.z;
            tMax.z += tDelta.z;
        }
    }
    return false;
}

constexpr bool World::IsInRadius(const glm::ivec2& entity, const glm::ivec2& origin, int32_t radius)
{
    glm::ivec2 d = entity - origin;
    return d.x * d.x + d.y * d.y <= radius * radius;
}

std::array<std::shared_ptr<Chunk>, 9> World::Gather3x3(const glm::ivec2& center) const
{
    std::array<std::shared_ptr<Chunk>, 9> grid;
    for (int32_t dz = -1; dz <= 1; dz++) {
        for (int32_t dx = -1; dx <= 1; dx++) {
            grid[(dz + 1) * 3 + (dx + 1)] = m_Chunks.find(center + glm::ivec2(dx, dz))->second.chunk;
        }
    }
    return grid;
}

std::array<const Chunk*, 9> World::AsPointers(const std::array<std::shared_ptr<Chunk>, 9>& grid)
{
    std::array<const Chunk*, 9> pointers;
    for (size_t i = 0; i < 9; i++) {
        pointers[i] = grid[i].get();
    }
    return pointers;
}

void World::DrainResults()
{
    for (auto& result : m_TerrainResults.Drain()) {
        m_PendingTerrain.erase(result.position);
        m_Chunks.try_emplace(result.position, std::move(result.chunk), ChunkState::k_TerrainReady);
    }

    for (const auto& position : m_LightResults.Drain()) {
        m_PendingLight.erase(position);
        auto it = m_Chunks.find(position);
        if (it != m_Chunks.end() && it->second.state == ChunkState::k_TerrainReady) {
            it->second.state = ChunkState::k_LightReady;
        }
    }

    // Uploading a finished mesh to the GPU is the expensive part, so cap how
    // many we take per frame; the rest wait in the queue for the next frames.
    size_t meshBudget = static_cast<size_t>(std::max(m_MaxMeshUploadsPerFrame, 0));
    for (auto& result : m_MeshResults.DrainUpTo(meshBudget)) {
        m_PendingMesh.erase(result.position);
        auto it = m_Chunks.find(result.position);
        if (it == m_Chunks.end()) {
            continue;
        }
        it->second.mesh = std::make_unique<ChunkMesh>(result.data);
        it->second.state = ChunkState::k_MeshReady;
    }
}

void World::InvalidateChunk(const glm::ivec2& chunkPosition)
{
    auto it = m_Chunks.find(chunkPosition);
    if (it == m_Chunks.end()) {
        return;
    }

    // Drop back to the terrain stage so Update re-dispatches lighting and
    // meshing. The stale mesh keeps rendering until the new one is ready.
    it->second.state = ChunkState::k_TerrainReady;
    m_PendingLight.erase(chunkPosition);
    m_PendingMesh.erase(chunkPosition);
}

bool World::HasAllTerrainNeighbours(const glm::ivec2& chunkPosition) const
{
    for (int32_t dz = -1; dz <= 1; dz++) {
        for (int32_t dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dz == 0) {
                continue;
            }
            if (!m_Chunks.count(chunkPosition + glm::ivec2(dx, dz))) {
                return false;
            }
        }
    }
    return true;
}

bool World::HasAllLitNeighbours(const glm::ivec2& chunkPosition) const
{
    for (int32_t dz = -1; dz <= 1; dz++) {
        for (int32_t dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dz == 0) {
                continue;
            }
            auto it = m_Chunks.find(chunkPosition + glm::ivec2(dx, dz));
            if (it == m_Chunks.end() || it->second.state == ChunkState::k_TerrainReady) {
                return false;
            }
        }
    }
    return true;
}

} // namespace Krafter
