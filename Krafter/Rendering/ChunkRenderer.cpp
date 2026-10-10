#include <algorithm>
#include <cstring>

#include "imgui.h"

#include "Krafter/Rendering/ChunkRenderer.h"
#include "Krafter/Rendering/RenderView.h"
#include "Krafter/World/Coords.h"
#include "Krafter/World/World.h"

namespace Krafter {

namespace {

void EnsureCapacity(Renderer& renderer, OwnedBuffer& buffer, void*& mapped, size_t& capacity, size_t count,
    size_t elementSize, VkBufferUsageFlags usage)
{
    if (count <= capacity) {
        return;
    }
    capacity = std::max<size_t>({ count, capacity * 2, 256 });
    buffer = renderer.CreateHostVisibleBuffer(capacity * elementSize, usage, &mapped);
}

}

ChunkRenderer::ChunkRenderer(Renderer& renderer, const World& world, JobSystem& jobs)
    : m_Renderer(renderer)
    , m_World(world)
    , m_JobSystem(jobs)
{
}

void ChunkRenderer::Update(const glm::vec3& cameraPosition, float deltaTime)
{
    for (auto it = m_Meshes.begin(); it != m_Meshes.end();) {
        if (m_World.HasChunk(it->first)) {
            ++it;
        } else {
            it = m_Meshes.erase(it);
        }
    }

    DrainResults(deltaTime);

    const glm::ivec2 origin = ToChunkPosition(
        glm::ivec3(glm::floor(cameraPosition.x), 0, glm::floor(cameraPosition.z)));
    const int32_t radius = m_World.GetRenderDistance();

    for (int32_t x = -radius; x <= radius; x++) {
        for (int32_t z = -radius; z <= radius; z++) {
            const glm::ivec2 position = origin + glm::ivec2(x, z);
            if (!World::IsInRadius(position, origin, radius) || !m_World.IsReadyToMesh(position)) {
                continue;
            }

            const uint64_t version = m_World.GetMeshVersion(position);
            auto [it, created] = m_Meshes.try_emplace(position);
            MeshRecord& record = it->second;
            if (created) {
                record.firstVersion = version;
            }
            if (record.builtVersion == version || record.dispatchedVersion == version) {
                continue;
            }

            record.dispatchedVersion = version;
            m_JobSystem.Dispatch([this, position, version, grid = m_World.Snapshot3x3(position)] {
                ChunkMeshData data = ChunkMesh::Compute(World::AsPointers(grid), position, m_World.GetClimate());
                m_MeshResults.Push({ position, std::move(data), version });
            });
        }
    }
}

void ChunkRenderer::DrainResults(float deltaTime)
{
    auto isUseful = [this](const MeshResult& result) -> MeshRecord* {
        auto it = m_Meshes.find(result.position);
        if (it == m_Meshes.end()) {
            return nullptr;
        }
        MeshRecord& record = it->second;
        const bool fresh = result.version >= record.firstVersion && result.version > record.builtVersion;
        return fresh ? &record : nullptr;
    };
    auto upload = [this](MeshRecord& record, const MeshResult& result) {
        record.mesh = std::make_unique<ChunkMesh>(result.data, m_Renderer.GetGeometry());
        record.builtVersion = result.version;
    };

    for (MeshResult& result : m_MeshResults.Drain()) {
        MeshRecord* record = isUseful(result);
        if (record == nullptr) {
            continue;
        }
        if (record->mesh) {
            upload(*record, result);
        } else {
            m_NewMeshes.push_back(std::move(result));
        }
    }

    m_UploadCredit += m_UploadsPerSecond * deltaTime;
    size_t budget = static_cast<size_t>(m_UploadCredit);
    m_UploadCredit -= static_cast<float>(budget);
    while (budget > 0 && !m_NewMeshes.empty()) {
        MeshResult result = std::move(m_NewMeshes.front());
        m_NewMeshes.pop_front();
        if (MeshRecord* record = isUseful(result)) {
            upload(*record, result);
            budget--;
        }
    }
}

void ChunkRenderer::PrepareFrame(const RenderView& view)
{
    m_Visible.clear();
    m_Commands.clear();
    m_Instances.clear();
    for (std::vector<DrawRun>& runs : m_Runs) {
        runs.clear();
    }
    m_CulledCount = 0;

    for (const auto& [position, record] : m_Meshes) {
        if (!record.mesh || !record.mesh->HasGeometry()) {
            continue;
        }
        const ChunkMesh& mesh = *record.mesh;

        const glm::vec3 origin = view.Relative(glm::ivec3(position.x * Chunk::k_Width, 0, position.y * Chunk::k_Width));
        const glm::vec3 boxMin = origin + glm::vec3(0.0f, mesh.GetMinY(), 0.0f);
        const glm::vec3 boxMax = origin + glm::vec3(Chunk::k_Width, mesh.GetMaxY(), Chunk::k_Width);
        if (m_FrustumCulling && !view.frustum.IntersectsBox(boxMin, boxMax)) {
            m_CulledCount++;
            continue;
        }

        const glm::vec3 center = (boxMin + boxMax) * 0.5f;
        m_Visible.push_back({ &mesh, static_cast<uint32_t>(m_Instances.size()), glm::dot(center, center) });
        m_Instances.push_back({ origin, 0.0f });
    }

    std::sort(m_Visible.begin(), m_Visible.end(), [](const VisibleChunk& a, const VisibleChunk& b) {
        if (a.mesh->GetPage() != b.mesh->GetPage()) {
            return a.mesh->GetPage() < b.mesh->GetPage();
        }
        return a.distanceSq < b.distanceSq;
    });
    EmitPass(ChunkPass::k_Opaque);
    EmitPass(ChunkPass::k_Cutout);

    std::sort(m_Visible.begin(), m_Visible.end(), [](const VisibleChunk& a, const VisibleChunk& b) {
        return a.distanceSq > b.distanceSq;
    });
    EmitPass(ChunkPass::k_Translucent);
    EmitPass(ChunkPass::k_Transparent);

    UploadFrame();
}

void ChunkRenderer::EmitPass(ChunkPass pass)
{
    std::vector<DrawRun>& runs = m_Runs[static_cast<size_t>(pass)];
    for (const VisibleChunk& chunk : m_Visible) {
        const ChunkMesh::Part& part = chunk.mesh->GetPart(pass);
        if (part.indexCount == 0) {
            continue;
        }

        const uint32_t index = static_cast<uint32_t>(m_Commands.size());
        m_Commands.push_back({ part.indexCount, 1, part.firstIndex, part.vertexOffset, chunk.instance });

        const uint32_t page = chunk.mesh->GetPage();
        if (!runs.empty() && runs.back().page == page && runs.back().first + runs.back().count == index) {
            runs.back().count++;
        } else {
            runs.push_back({ page, index, 1 });
        }
    }
}

void ChunkRenderer::UploadFrame()
{
    Renderer& renderer = m_Renderer;
    m_FrameIndex = renderer.GetCurrentFrameIndex();
    FrameBuffers& frame = m_Frames[m_FrameIndex];

    if (!m_Commands.empty()) {
        EnsureCapacity(m_Renderer, frame.commands, frame.commandsMapped, frame.commandCapacity, m_Commands.size(),
            sizeof(VkDrawIndexedIndirectCommand), VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
        const size_t bytes = m_Commands.size() * sizeof(VkDrawIndexedIndirectCommand);
        std::memcpy(frame.commandsMapped, m_Commands.data(), bytes);
        renderer.FlushHostBuffer(frame.commands, 0, bytes);
    }
    if (!m_Instances.empty()) {
        EnsureCapacity(m_Renderer, frame.instances, frame.instancesMapped, frame.instanceCapacity, m_Instances.size(),
            sizeof(WorldRenderer::ChunkInstance), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        const size_t bytes = m_Instances.size() * sizeof(WorldRenderer::ChunkInstance);
        std::memcpy(frame.instancesMapped, m_Instances.data(), bytes);
        renderer.FlushHostBuffer(frame.instances, 0, bytes);
    }
}

void ChunkRenderer::RenderPass(ChunkPass pass, WorldRenderer& renderer, const RenderView& view, const Sky& sky) const
{
    const std::vector<DrawRun>& runs = m_Runs[static_cast<size_t>(pass)];
    if (runs.empty()) {
        return;
    }

    renderer.BeginChunkPass(pass, view, sky);

    Renderer& gpu = m_Renderer;
    VkCommandBuffer cmd = gpu.GetCommandBuffer();
    const GeometryArena& arena = gpu.GetGeometry();
    const FrameBuffers& frame = m_Frames[m_FrameIndex];
    const bool multiDraw = gpu.SupportsMultiDrawIndirect();
    const uint32_t maxPerCall = std::max(gpu.GetMaxDrawIndirectCount(), 1u);

    for (const DrawRun& run : runs) {
        const VkBuffer buffers[2] = { arena.GetBuffer(run.page), frame.instances.GetHandle() };
        const VkDeviceSize offsets[2] = { 0, 0 };
        vkCmdBindVertexBuffers(cmd, 0, 2, buffers, offsets);
        vkCmdBindIndexBuffer(cmd, buffers[0], 0, VK_INDEX_TYPE_UINT32);

        if (multiDraw) {
            for (uint32_t done = 0; done < run.count;) {
                const uint32_t count = std::min(run.count - done, maxPerCall);
                vkCmdDrawIndexedIndirect(cmd, frame.commands.GetHandle(),
                    (run.first + done) * sizeof(VkDrawIndexedIndirectCommand), count,
                    sizeof(VkDrawIndexedIndirectCommand));
                done += count;
            }
        } else {
            for (uint32_t i = run.first; i < run.first + run.count; i++) {
                const VkDrawIndexedIndirectCommand& draw = m_Commands[i];
                vkCmdDrawIndexed(cmd, draw.indexCount, draw.instanceCount, draw.firstIndex, draw.vertexOffset,
                    draw.firstInstance);
            }
        }
    }
}

void ChunkRenderer::RenderImGui()
{
    ImGui::Text("Chunk Meshes");
    ImGui::SliderFloat("Mesh Uploads / sec", &m_UploadsPerSecond, 1.0f, 2000.0f, "%.0f");
    ImGui::Checkbox("Frustum Culling", &m_FrustumCulling);

    size_t meshed = 0;
    size_t pending = 0;
    for (const auto& [position, record] : m_Meshes) {
        meshed += record.mesh != nullptr;
        pending += record.IsPending();
    }
    ImGui::Text("Meshes: %zu, pending %zu", meshed, pending);

    size_t runs = 0;
    for (const std::vector<DrawRun>& passRuns : m_Runs) {
        runs += passRuns.size();
    }
    ImGui::Text("Visible %zu, culled %zu", m_Visible.size(), m_CulledCount);
    ImGui::Text("Draws %zu in %zu %s", m_Commands.size(), runs,
        m_Renderer.SupportsMultiDrawIndirect() ? "multi-draws" : "runs (no multi-draw)");

    const GeometryArena& arena = m_Renderer.GetGeometry();
    ImGui::Text("Geometry: %.1f / %.1f MB in %zu pages",
        arena.GetUsedBytes() / (1024.0 * 1024.0), arena.GetCapacityBytes() / (1024.0 * 1024.0), arena.GetPageCount());
}

}
