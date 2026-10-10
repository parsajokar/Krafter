#include <limits>
#include <vector>

#include "imgui.h"

#include "Krafter/World/BlockBehavior.h"
#include "Krafter/World/Coords.h"
#include "Krafter/World/Fluid.h"
#include "Krafter/World/Lighting.h"
#include "Krafter/World/World.h"

namespace Krafter {

namespace {

constexpr glm::ivec3 k_Neighbors[6] = {
    { 1, 0, 0 }, { -1, 0, 0 }, { 0, 0, 1 }, { 0, 0, -1 }, { 0, 1, 0 }, { 0, -1, 0 }
};

}

World::World(int32_t seed, JobSystem& jobs)
    : m_JobSystem(jobs)
    , m_Generator(seed)
    , m_FallingSystem(*this)
    , m_FluidSystem(*this)
    , m_DropSystem(*this)
{
}

World::~World() = default;

void World::Tick(float step, const glm::vec3& playerPosition, ItemCollector& collector)
{
    ProcessBlockUpdates();
    m_FallingSystem.Update(step);
    m_FluidSystem.Update(step);
    m_DropSystem.Tick(step, playerPosition, collector);
}

void World::Update(const glm::vec3& cameraPosition, float deltaTime)
{
    DrainResults();

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
            if (auto stored = m_ModifiedChunks.find(position); stored != m_ModifiedChunks.end()) {
                m_Chunks.try_emplace(position, std::move(stored->second), NextVersion());
                m_ModifiedChunks.erase(stored);
                continue;
            }
            m_PendingTerrain.insert(position);
            m_JobSystem.Dispatch([this, position, gen = m_Generation] {
                auto chunk = std::make_shared<Chunk>(position);
                m_Generator.Generate(*chunk, position);
                chunk->ClearModified();
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
            if (it == m_Chunks.end() || it->second.dispatchedLightVersion == it->second.lightVersion) {
                continue;
            }
            if (!HasAllTerrainNeighbours(position)) {
                continue;
            }

            it->second.dispatchedLightVersion = it->second.lightVersion;
            m_JobSystem.Dispatch([this, position, version = it->second.lightVersion, grid = Snapshot3x3(position)] {
                const std::array<const Chunk*, 9> pointers = AsPointers(grid);
                std::vector<uint8_t> sky(Chunk::k_Volume);
                std::vector<uint8_t> block(Chunk::k_Volume);
                ComputeSkyLight(sky, pointers);
                ComputeBlockLight(block, pointers);
                m_LightResults.Push({ position, ChunkLight::Pack(sky, block), version });
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
            if (it->second.chunk->IsModified()) {
                m_ModifiedChunks[it->first] = std::move(it->second.chunk);
            }
            it = m_Chunks.erase(it);
            removed++;
        } else {
            ++it;
        }
    }
}

void World::RenderImGui()
{
    ImGui::Text("World");
    ImGui::SliderInt("Render Distance", &m_RenderDistance, 1, 32);
    ImGui::SliderFloat("Chunk Removals / sec", &m_ChunkRemovalsPerSecond, 1.0f, 2000.0f, "%.0f");
    if (ImGui::Button("Reload Chunks")) {
        Reload();
    }

    size_t lit = 0;
    size_t pendingLight = 0;
    for (const auto& [position, record] : m_Chunks) {
        lit += record.IsLit();
        pendingLight += record.litVersion != record.lightVersion;
    }

    ImGui::Text("Workers: %zu", m_JobSystem.WorkerCount());
    ImGui::Text("Chunks: %zu (lit %zu)", m_Chunks.size(), lit);
    ImGui::Text("Modified chunks kept: %zu", m_ModifiedChunks.size());

    size_t bytes = 0;
    for (const auto& [position, record] : m_Chunks) {
        bytes += record.chunk->GetMemoryBytes();
    }
    for (const auto& [position, chunk] : m_ModifiedChunks) {
        bytes += chunk->GetMemoryBytes();
    }
    const size_t chunkCount = m_Chunks.size() + m_ModifiedChunks.size();
    ImGui::Text("Chunk memory: %.1f MB (%.0f KB per chunk)", bytes / (1024.0 * 1024.0),
        chunkCount > 0 ? bytes / 1024.0 / static_cast<double>(chunkCount) : 0.0);
    ImGui::Text("Pending: terrain %zu, light %zu, block updates %zu",
        m_PendingTerrain.size(), pendingLight, m_BlockUpdates.size());
}

Block World::GetBlock(const glm::ivec3& worldPosition) const
{
    if (worldPosition.y < 0 || worldPosition.y >= Chunk::k_Height) {
        return Block::k_Air;
    }

    auto it = m_Chunks.find(ToChunkPosition(worldPosition));
    if (it == m_Chunks.end()) {
        return Block::k_Air;
    }
    return it->second.chunk->GetBlock(ToLocalPosition(worldPosition));
}

uint8_t World::GetFluid(const glm::ivec3& worldPosition) const
{
    if (worldPosition.y < 0 || worldPosition.y >= Chunk::k_Height) {
        return 0;
    }

    auto it = m_Chunks.find(ToChunkPosition(worldPosition));
    if (it == m_Chunks.end()) {
        return 0;
    }
    return it->second.chunk->GetFluid(ToLocalPosition(worldPosition));
}

bool World::IsChunkLoaded(const glm::vec3& worldPosition) const
{
    return HasChunk(ToChunkPosition(glm::ivec3(glm::floor(worldPosition))));
}

bool World::SetBlock(const glm::ivec3& worldPosition, Block block)
{
    return WriteCell(worldPosition, block, IsFluid(block) ? k_FluidSource : 0);
}

bool World::SetFluid(const glm::ivec3& worldPosition, Block fluid, uint8_t level)
{
    return WriteCell(worldPosition, fluid, level);
}

bool World::WriteCell(const glm::ivec3& worldPosition, Block block, uint8_t fluid)
{
    if (worldPosition.y < 0 || worldPosition.y >= Chunk::k_Height) {
        return false;
    }

    auto it = m_Chunks.find(ToChunkPosition(worldPosition));
    if (it == m_Chunks.end()) {
        return false;
    }

    Chunk& chunk = *it->second.chunk;
    const glm::ivec3 local = ToLocalPosition(worldPosition);
    const Block previous = chunk.GetBlock(local);
    if (previous == block && chunk.GetFluid(local) == fluid) {
        return true;
    }
    chunk.SetBlock(local, block);
    chunk.SetFluid(local, fluid);
    MarkChanged(worldPosition, previous, block);

    if (previous == block) {
        return true;
    }

    if (const auto onRemoved = BehaviorOf(previous).onRemoved) {
        onRemoved(*this, worldPosition, previous, block);
    }
    if (const auto onPlaced = BehaviorOf(block).onPlaced) {
        onPlaced(*this, worldPosition, block);
    }
    for (const glm::ivec3& dir : k_Neighbors) {
        m_BlockUpdates.push_back(worldPosition + dir);
    }
    return true;
}

void World::MarkChanged(const glm::ivec3& worldPosition, Block previous, Block block)
{
    const glm::ivec2 chunkPosition = ToChunkPosition(worldPosition);

    if (BlocksLight(previous) != BlocksLight(block) || LightEmission(previous) != LightEmission(block)) {
        if (!UpdateLightIncrementally(worldPosition, previous)) {
            for (int32_t dz = -1; dz <= 1; dz++) {
                for (int32_t dx = -1; dx <= 1; dx++) {
                    InvalidateChunk(chunkPosition + glm::ivec2(dx, dz));
                }
            }
            return;
        }
    }

    ForEachMeshSeeing(worldPosition, [this](const glm::ivec2& chunk) { RemeshChunk(chunk); });
}

bool World::UpdateLightIncrementally(const glm::ivec3& worldPosition, Block previous)
{
    const glm::ivec2 center = ToChunkPosition(worldPosition);
    std::array<Chunk*, 9> area;
    for (int32_t dz = -1; dz <= 1; dz++) {
        for (int32_t dx = -1; dx <= 1; dx++) {
            auto it = m_Chunks.find(center + glm::ivec2(dx, dz));
            if (it == m_Chunks.end() || !it->second.IsLit() || it->second.litVersion != it->second.lightVersion) {
                return false;
            }
            area[(dz + 1) * 3 + (dx + 1)] = it->second.chunk.get();
        }
    }

    std::unordered_set<glm::ivec2> remesh;
    UpdateLight(area, worldPosition, previous, [&](const glm::ivec3& cell) {
        ForEachMeshSeeing(cell, [&](const glm::ivec2& chunk) { remesh.insert(chunk); });
    });
    for (const glm::ivec2& chunk : remesh) {
        RemeshChunk(chunk);
    }
    return true;
}

void World::RemeshNeighboursSeeingChange(const Chunk& before, const Chunk& after)
{
    const glm::ivec2 position = after.GetPosition();
    const int32_t top = std::min(Chunk::k_Height, std::max(before.GetTopY(), after.GetTopY()) + Chunk::k_SectionHeight);
    for (int32_t dz = -1; dz <= 1; dz++) {
        for (int32_t dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dz == 0) {
                continue;
            }
            const int32_t x0 = dx > 0 ? Chunk::k_Width - 1 : 0;
            const int32_t x1 = dx < 0 ? 0 : Chunk::k_Width - 1;
            const int32_t z0 = dz > 0 ? Chunk::k_Width - 1 : 0;
            const int32_t z1 = dz < 0 ? 0 : Chunk::k_Width - 1;
            bool changed = false;
            for (int32_t y = 0; y < top && !changed; y++) {
                for (int32_t z = z0; z <= z1 && !changed; z++) {
                    for (int32_t x = x0; x <= x1 && !changed; x++) {
                        const glm::ivec3 cell(x, y, z);
                        changed = before.GetSkyLight(cell) != after.GetSkyLight(cell)
                            || before.GetBlockLight(cell) != after.GetBlockLight(cell);
                    }
                }
            }
            if (changed) {
                RemeshChunk(position + glm::ivec2(dx, dz));
            }
        }
    }
}

void World::ProcessBlockUpdates()
{
    for (size_t processed = 0; processed < k_MaxBlockUpdatesPerTick && !m_BlockUpdates.empty(); processed++) {
        const glm::ivec3 cell = m_BlockUpdates.front();
        m_BlockUpdates.pop_front();

        if (!IsLoadedAround(cell)) {
            continue;
        }

        const Block block = GetBlock(cell);
        const BlockBehavior& behavior = BehaviorOf(block);
        if (behavior.canSurvive && !behavior.canSurvive(*this, cell)) {
            if (behavior.onUnsupported) {
                behavior.onUnsupported(*this, cell, block);
            } else {
                SetBlock(cell, Block::k_Air);
            }
        } else if (behavior.onNeighbourChanged) {
            behavior.onNeighbourChanged(*this, cell, block);
        }
    }
}

bool World::IsNeighbourhoodLoaded(const glm::ivec2& chunkPosition) const
{
    for (int32_t dz = -1; dz <= 1; dz++) {
        for (int32_t dx = -1; dx <= 1; dx++) {
            if (!m_Chunks.count(chunkPosition + glm::ivec2(dx, dz))) {
                return false;
            }
        }
    }
    return true;
}

bool World::IsLoadedAround(const glm::ivec3& cell) const
{
    return IsNeighbourhoodLoaded(ToChunkPosition(cell));
}

bool World::IsLoadedOrKept(const glm::ivec3& cell) const
{
    const glm::ivec2 chunkPosition = ToChunkPosition(cell);
    return m_Chunks.count(chunkPosition) || m_ModifiedChunks.count(chunkPosition);
}

bool World::PlaceBlock(const glm::ivec3& worldPosition, Block block)
{
    if (block == Block::k_Air) {
        return false;
    }

    const BlockBehavior& behavior = BehaviorOf(block);
    if (behavior.needsEmptyCell) {
        const Block target = GetBlock(worldPosition);
        if (target != Block::k_Air && !IsFluid(target)) {
            return false;
        }
    }
    if (behavior.canSurvive && !behavior.canSurvive(*this, worldPosition)) {
        return false;
    }

    return SetBlock(worldPosition, block);
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

bool World::IsReadyToMesh(const glm::ivec2& chunkPosition) const
{
    auto it = m_Chunks.find(chunkPosition);
    return it != m_Chunks.end() && it->second.IsLit() && HasAllLitNeighbours(chunkPosition);
}

std::array<std::shared_ptr<const Chunk>, 9> World::Snapshot3x3(const glm::ivec2& center) const
{
    std::array<std::shared_ptr<const Chunk>, 9> grid;
    for (int32_t dz = -1; dz <= 1; dz++) {
        for (int32_t dx = -1; dx <= 1; dx++) {
            grid[(dz + 1) * 3 + (dx + 1)] = m_Chunks.at(center + glm::ivec2(dx, dz)).chunk->Snapshot();
        }
    }
    return grid;
}

std::array<const Chunk*, 9> World::AsPointers(const std::array<std::shared_ptr<const Chunk>, 9>& grid)
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
        if (result.generation != m_Generation) {
            continue;
        }
        m_PendingTerrain.erase(result.position);
        m_Chunks.try_emplace(result.position, std::move(result.chunk), NextVersion());
    }

    for (auto& result : m_LightResults.Drain()) {
        auto it = m_Chunks.find(result.position);
        if (it == m_Chunks.end()) {
            continue;
        }
        ChunkRecord& record = it->second;
        if (result.version < record.createdVersion || result.version <= record.litVersion) {
            continue;
        }
        const bool wasLit = record.IsLit();
        const std::shared_ptr<const Chunk> before = record.chunk->Snapshot();
        record.chunk->SetLight(std::move(result.light));
        record.litVersion = result.version;
        record.meshVersion = NextVersion();
        if (wasLit) {
            RemeshNeighboursSeeingChange(*before, *record.chunk);
        }
    }
}

void World::Reload()
{
    m_Generation++;
    for (auto& [position, record] : m_Chunks) {
        if (record.chunk->IsModified()) {
            m_ModifiedChunks[position] = std::move(record.chunk);
        }
    }
    m_Chunks.clear();
    m_PendingTerrain.clear();
}

void World::InvalidateChunk(const glm::ivec2& chunkPosition)
{
    auto it = m_Chunks.find(chunkPosition);
    if (it == m_Chunks.end()) {
        return;
    }

    ChunkRecord& record = it->second;
    record.lightVersion = NextVersion();
    record.meshVersion = NextVersion();
}

void World::RemeshChunk(const glm::ivec2& chunkPosition)
{
    auto it = m_Chunks.find(chunkPosition);
    if (it == m_Chunks.end()) {
        return;
    }
    it->second.meshVersion = NextVersion();
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
            if (it == m_Chunks.end() || !it->second.IsLit()) {
                return false;
            }
        }
    }
    return true;
}

}
