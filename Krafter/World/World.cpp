#include <limits>
#include <vector>

#include "imgui.h"

#include "Krafter/Renderer/WorldRenderer.h"
#include "Krafter/World/Coords.h"
#include "Krafter/World/Fluid.h"
#include "Krafter/World/Lighting.h"
#include "Krafter/World/Sky.h"
#include "Krafter/World/World.h"

namespace Krafter {

namespace {

constexpr glm::ivec3 k_Neighbors[6] = {
    { 1, 0, 0 }, { -1, 0, 0 }, { 0, 0, 1 }, { 0, 0, -1 }, { 0, 1, 0 }, { 0, -1, 0 }
};

}

World::World(int32_t seed)
    : m_Generator(seed)
    , m_FallingSystem(*this)
    , m_FluidSystem(*this)
    , m_DropSystem(*this)
{
}

World::~World() = default;

void World::Update(const glm::vec3& cameraPosition, float deltaTime)
{
    DrainResults(deltaTime);
    m_FallingSystem.Update(deltaTime);
    m_FluidSystem.Update(deltaTime);

    m_DropSystem.Update(deltaTime, cameraPosition);

    glm::ivec2 origin = ToChunkPosition(
        glm::ivec3(glm::floor(cameraPosition.x), 0, glm::floor(cameraPosition.z)));

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
            m_JobSystem.Dispatch([this, position, gen = m_Generation] {
                auto chunk = std::make_shared<Chunk>(position);
                m_Generator.Generate(*chunk, position);
                m_TerrainResults.Push({ position, std::move(chunk), gen });
            });
        }
    }

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
            m_JobSystem.Dispatch([this, position, gen = m_Generation, grid = Gather3x3(position)] {
                const std::array<const Chunk*, 9> pointers = AsPointers(grid);
                ComputeSkyLight(*grid[4], pointers);
                ComputeBlockLight(*grid[4], pointers);
                m_LightResults.Push({ position, gen });
            });
        }
    }

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
            m_JobSystem.Dispatch([this, position, gen = m_Generation, grid = Gather3x3(position)] {
                ChunkMeshData data = ChunkMesh::Compute(AsPointers(grid), position);
                m_MeshResults.Push({ position, std::make_unique<ChunkMesh>(data), gen });
            });
        }
    }

    m_RemovalCredit += m_ChunkRemovalsPerSecond * deltaTime;
    int32_t removalBudget = static_cast<int32_t>(m_RemovalCredit);
    m_RemovalCredit -= static_cast<float>(removalBudget);

    int32_t removed = 0;
    for (auto it = m_Chunks.begin();
         it != m_Chunks.end() && removed < removalBudget;) {
        if (!IsInRadius(it->first, origin, m_RenderDistance + 2)) {
            it = m_Chunks.erase(it);
            removed++;
        } else {
            ++it;
        }
    }
}

void World::Render(WorldRenderer& renderer, const glm::mat4& viewProjection, const Sky& sky)
{
    renderer.SetCullFace(true);
    for (const auto& [position, record] : m_Chunks) {
        if (record.mesh) {
            renderer.RenderChunkOpaque(*record.mesh, viewProjection, sky);
        }
    }
    renderer.SetCullFace(false);

    m_DropSystem.Render(renderer, viewProjection);

    for (const auto& [position, record] : m_Chunks) {
        if (record.mesh) {
            renderer.RenderChunkCross(*record.mesh, viewProjection, sky);
        }
    }

    renderer.SetBlend(true);
    renderer.SetDepthMask(false);
    for (const auto& [position, record] : m_Chunks) {
        if (record.mesh) {
            renderer.RenderChunkTranslucent(*record.mesh, viewProjection, sky);
        }
    }
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
    ImGui::Text("World");
    ImGui::SliderInt("Render Distance", &m_RenderDistance, 1, 32);
    ImGui::SliderFloat("Chunk Uploads / sec", &m_ChunkUploadsPerSecond, 1.0f, 2000.0f, "%.0f");
    ImGui::SliderFloat("Chunk Removals / sec", &m_ChunkRemovalsPerSecond, 1.0f, 2000.0f, "%.0f");
    if (ImGui::Button("Reload Chunks")) {
        Reload();
    }

    size_t meshed = 0;
    for (const auto& [position, record] : m_Chunks) {
        if (record.state == ChunkState::k_MeshReady) {
            meshed++;
        }
    }

    ImGui::Text("Workers: %zu", m_JobSystem.WorkerCount());
    ImGui::Text("Chunks: %zu (meshed %zu)", m_Chunks.size(), meshed);
    ImGui::Text("Pending: terrain %zu, light %zu, mesh %zu",
        m_PendingTerrain.size(), m_PendingLight.size(), m_PendingMesh.size());
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
    const glm::ivec3 localPosition = ToLocalPosition(worldPosition);
    const Block previous = chunk.GetBlock(localPosition);
    chunk.SetBlock(localPosition, block);
    chunk.SetFluid(localPosition, IsFluid(block) ? k_FluidSource : 0);

    std::vector<glm::ivec3> topplingCactus;
    for (int32_t y = worldPosition.y + 1; y < Chunk::k_Height; y++) {
        const glm::ivec3 cell(worldPosition.x, y, worldPosition.z);
        const glm::ivec3 local = ToLocalPosition(cell);
        const Block fragile = chunk.GetBlock(local);
        if (!IsPlant(fragile) && fragile != Block::k_Cactus) {
            break;
        }
        const glm::ivec3 below = cell - glm::ivec3(0, 1, 0);
        const bool belowToppling = m_FallingSystem.IsFalling(below)
            || (!topplingCactus.empty() && topplingCactus.back() == below);
        if (IsOpaque(chunk.GetBlock(ToLocalPosition(below))) && !belowToppling) {
            break;
        }
        if (fragile == Block::k_Cactus) {
            topplingCactus.push_back(cell);
        } else {
            chunk.SetBlock(local, Block::k_Air);
        }
    }
    m_FallingSystem.Schedule(topplingCactus, worldPosition);

    if (IsOpaque(block)) {
        for (const glm::ivec3& side : k_HorizontalNeighbors) {
            BreakCactusColumn(worldPosition + side);
        }
    }

    if (IsNaturalTreePart(previous) && !IsNaturalTreePart(block)) {
        ChopFloatingTree(worldPosition);
    }

    BreakUnsupportedGems(worldPosition);

    if (IsFluid(block)) {
        m_FluidSystem.Schedule(worldPosition, block);
    }
    for (const glm::ivec3& dir : k_Neighbors) {
        const glm::ivec3 neighbour = worldPosition + dir;
        const Block neighbourBlock = GetBlock(neighbour);
        if (IsFluid(neighbourBlock)) {
            m_FluidSystem.Schedule(neighbour, neighbourBlock);
        }
    }

    for (int32_t dz = -1; dz <= 1; dz++) {
        for (int32_t dx = -1; dx <= 1; dx++) {
            InvalidateChunk(chunkPosition + glm::ivec2(dx, dz));
        }
    }
}

// An edit can touch the whole 3x3 neighbourhood, so every neighbour must be
// meshed with no light/mesh job in flight; otherwise a worker could be reading a
// chunk this edit mutates (a data race).
bool World::CanEdit(const glm::ivec2& chunkPosition) const
{
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

    auto isStanding = [this](const glm::ivec3& cell) {
        return IsNaturalTreePart(GetBlock(cell)) && !m_FallingSystem.IsFalling(cell);
    };

    std::unordered_set<glm::ivec3> visited;
    std::vector<glm::ivec3> floating;

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

    m_FallingSystem.Schedule(floating, brokenPosition);
}

void World::BreakUnsupportedGems(const glm::ivec3& changedPosition)
{
    for (const glm::ivec3& dir : k_Neighbors) {
        const glm::ivec3 gemPos = changedPosition + dir;
        if (gemPos.y < 0 || gemPos.y >= Chunk::k_Height) {
            continue;
        }
        const Block gem = GetBlock(gemPos);
        if (!IsGem(gem)) {
            continue;
        }

        bool supported = false;
        for (const glm::ivec3& side : k_Neighbors) {
            if (IsOpaque(GetBlock(gemPos + side))) {
                supported = true;
                break;
            }
        }
        if (!supported) {
            SpawnDrop(glm::vec3(gemPos) + 0.5f, DropFor(gem));
            SetBlock(gemPos, Block::k_Air);
        }
    }
}

void World::BreakCactusColumn(const glm::ivec3& worldPosition)
{
    std::vector<glm::ivec3> column;
    for (int32_t y = worldPosition.y; y < Chunk::k_Height; y++) {
        const glm::ivec3 cell(worldPosition.x, y, worldPosition.z);
        if (GetBlock(cell) != Block::k_Cactus) {
            break;
        }
        column.push_back(cell);
    }
    m_FallingSystem.Schedule(column, worldPosition);
}

void World::PlaceBlock(const glm::ivec3& worldPosition, Block block)
{
    if (block == Block::k_Air) {
        return;
    }

    if (IsPlant(block)) {
        const glm::ivec3 below(worldPosition.x, worldPosition.y - 1, worldPosition.z);
        const Block target = GetBlock(worldPosition);
        if (below.y < 0 || !IsOpaque(GetBlock(below)) || (target != Block::k_Air && !IsFluid(target))) {
            return;
        }
    }

    if (block == Block::k_Cactus) {
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

void World::DrainResults(float deltaTime)
{
    for (auto& result : m_TerrainResults.Drain()) {
        if (result.generation != m_Generation) {
            continue;
        }
        m_PendingTerrain.erase(result.position);
        m_Chunks.try_emplace(result.position, std::move(result.chunk), ChunkState::k_TerrainReady);
    }

    for (const auto& result : m_LightResults.Drain()) {
        if (result.generation != m_Generation) {
            continue;
        }
        m_PendingLight.erase(result.position);
        auto it = m_Chunks.find(result.position);
        if (it != m_Chunks.end() && it->second.state == ChunkState::k_TerrainReady) {
            it->second.state = ChunkState::k_LightReady;
        }
    }

    m_UploadCredit += m_ChunkUploadsPerSecond * deltaTime;
    const size_t meshBudget = static_cast<size_t>(m_UploadCredit);
    m_UploadCredit -= static_cast<float>(meshBudget);
    for (auto& result : m_MeshResults.DrainUpTo(meshBudget)) {
        if (result.generation != m_Generation) {
            continue;
        }
        m_PendingMesh.erase(result.position);
        auto it = m_Chunks.find(result.position);
        if (it == m_Chunks.end()) {
            continue;
        }
        it->second.mesh = std::move(result.mesh);
        it->second.state = ChunkState::k_MeshReady;
    }
}

void World::Reload()
{
    // Bump the epoch so any in-flight jobs from the previous epoch are discarded
    // when their results drain, then clear all loaded and pending chunks. The
    // regeneration is re-requested by the next Update.
    m_Generation++;
    m_Chunks.clear();
    m_PendingTerrain.clear();
    m_PendingLight.clear();
    m_PendingMesh.clear();
}

void World::InvalidateChunk(const glm::ivec2& chunkPosition)
{
    auto it = m_Chunks.find(chunkPosition);
    if (it == m_Chunks.end()) {
        return;
    }

    it->second.state = ChunkState::k_TerrainReady;
    m_PendingLight.erase(chunkPosition);
    m_PendingMesh.erase(chunkPosition);
}

void World::RemeshChunk(const glm::ivec2& chunkPosition)
{
    auto it = m_Chunks.find(chunkPosition);
    if (it == m_Chunks.end()) {
        return;
    }

    if (it->second.state == ChunkState::k_MeshReady) {
        it->second.state = ChunkState::k_LightReady;
    }
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

}
