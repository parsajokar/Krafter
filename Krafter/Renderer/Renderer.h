#pragma once

#include <cstdint>
#include <memory>

#include "Krafter/Camera.h"
#include "Krafter/Renderer/ChunkMesh.h"
#include "Krafter/Renderer/ShaderProgram.h"
#include "Krafter/Renderer/Texture.h"

namespace Krafter {

class Renderer {
public:
    static void Init(); // !!! MAKE SURE TO CALL SHUTDOWN
    static void Shutdown();

    inline static Camera* GetCamera()
    {
        return s_Renderer->m_Camera;
    }
    inline static void SetCamera(Camera* camera)
    {
        s_Renderer->m_Camera = camera;
    }

    static void Clear();
    static void RenderChunkMesh(const ChunkMesh& chunkMesh);
    static void RenderImGui();

private:
    static void ApiDebugCallback(
        uint32_t source,
        uint32_t type,
        uint32_t id,
        uint32_t severity,
        int32_t length,
        const char* message,
        const void* userParam);

    inline static Renderer* s_Renderer = nullptr;

    Renderer();

    const uint8_t* m_VersionName;
    const uint8_t* m_RendererName;

    Camera* m_Camera;

    std::unique_ptr<ShaderProgram> m_Program;
    std::unique_ptr<Texture2D> m_Texture;
};

} // namespace Krafter
