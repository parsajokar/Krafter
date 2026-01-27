#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>

#include "glm/glm.hpp"
#include "glm/gtx/hash.hpp"

#include "Krafter/Layer.h"
#include "Krafter/Renderer/Camera.h"
#include "Krafter/Renderer/ChunkMesh.h"
#include "Krafter/Renderer/ShaderProgram.h"
#include "Krafter/Renderer/Texture.h"

namespace Krafter {

class Renderer : public Layer {
public:
    inline static Renderer* Get()
    {
        return s_Instance;
    }

    Renderer();
    ~Renderer() = default;

    void OnAttach() override;
    void OnRender() override;
    void OnRenderImGui() override;

    inline Camera& GetCamera()
    {
        return *m_Camera;
    }

    void GenerateChunkMesh(const ChunkMap& chunkMap, const glm::ivec2& chunkPosition);
    void DeleteChunkMesh(const glm::ivec2& chunkPosition);

    void ClearBuffers();

private:
    static void ApiDebugCallback(
        uint32_t source,
        uint32_t type,
        uint32_t id,
        uint32_t severity,
        int32_t length,
        const char* message,
        const void* userParam);

    inline static Renderer* s_Instance = nullptr;

    const uint8_t* m_VersionName;
    const uint8_t* m_RendererName;

    Camera* m_Camera;

    std::shared_ptr<ShaderProgram> m_Program;
    std::shared_ptr<Texture2D> m_Texture;
    std::unordered_map<glm::ivec2, std::shared_ptr<ChunkMesh>> m_ChunkMeshes;
};

} // namespace Krafter
