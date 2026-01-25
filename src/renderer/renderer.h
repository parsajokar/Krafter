#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>

#include "glm/glm.hpp"
#include "glm/gtx/hash.hpp"

#include "layer.h"
#include "renderer/camera.h"
#include "renderer/chunk_mesh.h"
#include "renderer/shader_program.h"
#include "renderer/texture.h"

namespace Krafter {

class Renderer : public Layer {
public:
    inline static Renderer* Get()
    {
        return _instance;
    }

    Renderer();
    ~Renderer() = default;

    void OnAttach() override;
    void OnRender() override;
    void OnRenderImGui() override;

    inline Camera& GetCamera()
    {
        return *_camera;
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

    inline static Renderer* _instance = nullptr;

    const uint8_t* _versionName;
    const uint8_t* _rendererName;

    Camera* _camera;

    std::shared_ptr<ShaderProgram> _program;
    std::shared_ptr<Texture2D> _texture;
    std::unordered_map<glm::ivec2, std::shared_ptr<ChunkMesh>> _chunkMeshes;
};

} // namespace Krafter
