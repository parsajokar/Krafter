#include <algorithm>
#include <limits>

#include "imgui.h"

#include "Krafter/Renderer/WorldRenderer.h"
#include "Krafter/World/Biome.h"
#include "Krafter/World/Coords.h"
#include "Krafter/World/Lighting.h"
#include "Krafter/World/Sky.h"
#include "Krafter/World/World.h"

namespace Krafter {

World::World()
{
    Biome::LoadBiomes();
}

// m_JobSystem is declared last, so it is destroyed first: its workers stop and
// join before the result queues they push into are torn down.
World::~World() = default;

void World::Update(const glm::vec3& cameraPosition)
{
    DrainResults();

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

void World::SetBlock(const glm::ivec3& worldPosition, Block block)
{
    if (worldPosition.y < 0 || worldPosition.y >= Chunk::k_Height) {
        return;
    }

    glm::ivec2 chunkPosition = ToChunkPosition(worldPosition);
    auto it = m_Chunks.find(chunkPosition);
    if (it == m_Chunks.end() || !it->second.chunk) {
        return;
    }

    // Only edit settled terrain. A block on a chunk border changes its
    // neighbours' light and faces too, so the whole 3x3 must be meshed and have
    // no light/mesh job in flight; otherwise a worker could be reading a chunk
    // we are about to mutate or invalidate.
    for (int32_t dz = -1; dz <= 1; dz++) {
        for (int32_t dx = -1; dx <= 1; dx++) {
            glm::ivec2 neighbour = chunkPosition + glm::ivec2(dx, dz);
            auto nit = m_Chunks.find(neighbour);
            if (nit == m_Chunks.end() || nit->second.state != ChunkState::k_MeshReady) {
                return;
            }
            if (m_PendingLight.count(neighbour) || m_PendingMesh.count(neighbour)) {
                return;
            }
        }
    }

    Chunk& chunk = *it->second.chunk;
    chunk.SetBlock(ToLocalPosition(worldPosition), block);

    // A plant resting on the block we just changed loses its footing when that
    // block stops being solid ground, so break it (its cell is always in this
    // same column, hence this same chunk).
    const glm::ivec3 above(worldPosition.x, worldPosition.y + 1, worldPosition.z);
    if (above.y < Chunk::k_Height) {
        const glm::ivec3 local = ToLocalPosition(above);
        if (IsPlant(chunk.GetBlock(local)) && !IsOpaque(block)) {
            chunk.SetBlock(local, Block::k_Air);
        }
    }

    for (int32_t dz = -1; dz <= 1; dz++) {
        for (int32_t dx = -1; dx <= 1; dx++) {
            InvalidateChunk(chunkPosition + glm::ivec2(dx, dz));
        }
    }
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
