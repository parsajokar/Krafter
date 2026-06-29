#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "glm/glm.hpp"
#include "glm/gtx/hash.hpp"

#include "Krafter/Core/JobSystem.h"
#include "Krafter/Core/ResultQueue.h"
#include "Krafter/Renderer/ChunkMesh.h"
#include "Krafter/World/Block.h"
#include "Krafter/World/Chunk.h"

namespace Krafter {

class WorldRenderer;
class Sky;

class World {
public:
    // `seed` drives terrain generation: the climate/detail noise and the feature
    // placement hashing. The same seed always rebuilds the same world; 0 is the
    // original fixed world.
    World(int32_t seed);
    ~World();

    World(const World&) = delete;
    World& operator=(const World&) = delete;

    void Update(const glm::vec3& cameraPosition, float deltaTime);
    void Render(WorldRenderer& renderer, const glm::mat4& viewProjection, const Sky& sky);
    void RenderImGui();

    Block GetBlock(const glm::ivec3& worldPosition) const;
    void SetBlock(const glm::ivec3& worldPosition, Block block);

    // Whether the chunk column at this world position has its terrain generated.
    // Survival physics check this before applying gravity so the player rests in
    // mid-air over still-loading terrain instead of falling through into the void.
    bool IsChunkLoaded(const glm::vec3& worldPosition) const;

    // Places a held block, applying plant rules: foliage needs solid ground
    // below, and tall grass also claims the cell above for its upper half.
    // Invalid placements (no support, no room) are simply ignored.
    void PlaceBlock(const glm::ivec3& worldPosition, Block block);

    // Steps a ray through the voxel grid and reports the first targetable block
    // hit (solids and plants). outBefore is the empty cell just before the hit,
    // where a block is placed.
    bool RaycastBlock(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, glm::ivec3& outHit, glm::ivec3& outBefore) const;

    // Spawns a floating item drop of `block` at `position` (world space, the cell
    // centre of whatever was removed): it falls, settles on the ground, and waits
    // to be walked over. A no-op for k_Air. Used both for the block the player
    // mines and for each block a chopped tree or toppled cactus sheds.
    void SpawnDrop(const glm::vec3& position, Block block);

    // Hands over the blocks the player has just walked over and picked up this
    // update, clearing the queue. Each entry is one collectible block ready to be
    // stowed in the inventory; the drops themselves are spawned by SpawnDrop and
    // live in the world until collected.
    std::vector<Block> TakeDrops()
    {
        return std::move(m_PendingDrops);
    }

private:
    enum class ChunkState {
        k_TerrainReady,
        k_LightReady,
        k_MeshReady
    };

    struct ChunkRecord {
        std::shared_ptr<Chunk> chunk;
        std::unique_ptr<ChunkMesh> mesh;
        ChunkState state;

        ChunkRecord(std::shared_ptr<Chunk> chunk, ChunkState state)
            : chunk(std::move(chunk))
            , state(state)
        {
        }
    };

    struct TerrainResult {
        glm::ivec2 position;
        std::shared_ptr<Chunk> chunk;
    };

    struct MeshResult {
        glm::ivec2 position;
        ChunkMeshData data;
    };

    // The lighting pass writes directly into the chunk's storage, so a finished
    // job only needs to report which chunk became lit.
    using LightResult = glm::ivec2;

    // A block waiting to vanish as part of a gradual break (a chopped tree's
    // floating remains, a toppling cactus). It stays in the world until `delay`
    // counts down to zero, then it is cleared once its chunk is safe to edit.
    struct FallingBlock {
        glm::ivec3 cell;
        float delay;
    };

    // Seconds added per block of distance from the break: the break ripples
    // outward at a constant speed, so a bigger tree takes longer to fully fall.
    static constexpr float k_FallStep = 0.05f;

    // A floating item dropped into the world: it falls under gravity and settles
    // on the first solid block beneath it, then once the player is within
    // k_AttractRadius it gravitates toward the eye (after a short delay so it
    // isn't grabbed mid-pop) and is collected on reaching k_PickupRadius.
    struct ItemDrop {
        glm::vec3 position; // centre, world space
        glm::vec3 velocity;
        Block block;
        float age;   // seconds alive, gating pickup and despawn
        float phase; // bob offset so drops don't bob in lockstep
    };

    // Drop physics and collection tuning, in blocks and blocks/second.
    static constexpr float k_DropGravity = 24.0f;
    static constexpr float k_DropPopUp = 3.0f;    // initial upward pop on spawn
    static constexpr float k_DropPopOut = 1.2f;   // initial horizontal scatter
    static constexpr float k_DropHalfHeight = 0.125f; // rests this far above ground
    static constexpr float k_PickupDelay = 0.5f;   // ungrabbable while it pops out
    static constexpr float k_AttractRadius = 4.0f; // starts gravitating in within this of the eye
    static constexpr float k_PickupRadius = 0.75f; // finally collected within this of the eye
    static constexpr float k_AttractMinSpeed = 4.0f;  // pull speed at the magnet's edge
    static constexpr float k_AttractMaxSpeed = 16.0f; // pull speed just before pickup
    static constexpr float k_DropLifetime = 300.0f; // despawns if never collected
    // Drops home to the player's feet, not the eye the camera rides at, so they
    // converge at ground level. This mirrors the player's eye-above-feet height
    // (Player::k_EyeHeight) to turn the eye Update is given into a foot position.
    static constexpr float k_PlayerEyeHeight = 1.62f;
    static constexpr float k_DropVoidY = -16.0f;  // despawns if it falls this low
    static constexpr float k_DropBobSpeed = 3.0f;
    static constexpr float k_DropBobAmplitude = 0.1f;
    static constexpr float k_DropSize = 0.4f; // billboard side length, in blocks

    static constexpr bool IsInRadius(const glm::ivec2& entity, const glm::ivec2& origin, int32_t radius);

    // The 3x3 chunk neighbourhood around `center`, indexed (dz + 1) * 3 + (dx + 1)
    // with the centre at 4. Both the lighting and meshing jobs need it.
    std::array<std::shared_ptr<Chunk>, 9> Gather3x3(const glm::ivec2& center) const;
    static std::array<const Chunk*, 9> AsPointers(const std::array<std::shared_ptr<Chunk>, 9>& grid);

    void DrainResults();

    // Whether the chunk and its full 3x3 are meshed with no light/mesh job in
    // flight, so editing a block in it (which can touch its neighbours) won't
    // race a worker reading those chunks.
    bool CanEdit(const glm::ivec2& chunkPosition) const;

    // Breaks the cactus at a cell and every cactus stacked above it. Used when a
    // newly placed block ends up beside a cactus, which snaps it off.
    void BreakCactusColumn(const glm::ivec3& worldPosition);

    // After a tree block is broken at this cell, schedules every wood/leaf block
    // the break left stranded with no path of tree blocks back down to the
    // ground, the way chopping a tree in Terraria drops the part above the cut.
    void ChopFloatingTree(const glm::ivec3& brokenPosition);

    // Spreads the removal of `cells` across k_FallDuration so they don't all
    // vanish at once: each clears on a timer that grows with its distance from
    // `origin`, so the break ripples outward. Cells already falling are skipped.
    void ScheduleFall(const std::vector<glm::ivec3>& cells, const glm::ivec3& origin);

    // Clears the scheduled falling blocks whose timer has run out, once their
    // chunk is safe to edit, then re-meshes whatever chunks they leave behind.
    void UpdateFallingBlocks(float deltaTime);

    // Advances every floating drop: gravity and ground settling, then collection
    // into m_PendingDrops when one lingers within reach of `cameraPosition`, and
    // despawning once it ages out or falls into the void.
    void UpdateDrops(float deltaTime, const glm::vec3& cameraPosition);

    // Resets a chunk back to the terrain stage so the lighting and meshing
    // passes recompute it on the next update.
    void InvalidateChunk(const glm::ivec2& chunkPosition);

    // Both stages need the full 3x3 neighbourhood: lighting for block context,
    // meshing so border smooth-lighting and AO read settled neighbour light.
    bool HasAllTerrainNeighbours(const glm::ivec2& chunkPosition) const;
    bool HasAllLitNeighbours(const glm::ivec2& chunkPosition) const;

    std::unordered_map<glm::ivec2, ChunkRecord> m_Chunks;
    int32_t m_RenderDistance = 10;
    int32_t m_MaxMeshUploadsPerFrame = 8;

    std::unordered_set<glm::ivec2> m_PendingTerrain;
    std::unordered_set<glm::ivec2> m_PendingLight;
    std::unordered_set<glm::ivec2> m_PendingMesh;

    // Blocks counting down to removal, with a membership set so a later break
    // treats an already-doomed block as gone instead of scheduling it twice or
    // mistaking it for solid support.
    std::vector<FallingBlock> m_FallingBlocks;
    std::unordered_set<glm::ivec3> m_Falling;

    // Floating item drops live here until collected. m_PendingDrops holds the
    // blocks picked up this update, drained by the player each frame through
    // TakeDrops(). m_Time accumulates for the drop bob; m_LastCameraPosition is
    // the eye recorded each Update, used to billboard the drops in Render.
    std::vector<ItemDrop> m_Drops;
    std::vector<Block> m_PendingDrops;
    float m_Time = 0.0f;
    glm::vec3 m_LastCameraPosition = glm::vec3(0.0f);

    ResultQueue<TerrainResult> m_TerrainResults;
    ResultQueue<LightResult> m_LightResults;
    ResultQueue<MeshResult> m_MeshResults;

    JobSystem m_JobSystem;
};

} // namespace Krafter
