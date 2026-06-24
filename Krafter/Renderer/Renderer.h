#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "glm/glm.hpp"

#include "Krafter/Renderer/ChunkMesh.h"
#include "Krafter/Renderer/ShaderProgram.h"
#include "Krafter/Renderer/Texture.h"

#if defined(_MSC_VER)

#define STDCALL __stdcall

#elif defined(__GNUC__)

#if defined(__i386__)
#define STDCALL __attribute__((stdcall))
#else
#define STDCALL
#endif

#else

#define STDCALL

#endif

namespace Krafter {

class Sky;

class Renderer {
public:
    Renderer();
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void SetClearColor(const glm::vec3& color);
    void Clear();

    void RenderChunkOpaque(const ChunkMesh& chunkMesh, const glm::mat4& viewProjection, const Sky& sky);
    void RenderChunkTransparent(const ChunkMesh& chunkMesh, const glm::mat4& viewProjection, const Sky& sky);
    void SetDepthMask(bool enabled);
    void SetBlend(bool enabled);

    // Advances the animated water texture, swapping the current frame into the
    // atlas's water tile.
    void AnimateWater();
    void RenderBlockOutline(const glm::ivec3& blockPosition, const glm::mat4& viewProjection);
    void RenderImGui();

private:
    void BindChunkProgram(const glm::mat4& viewProjection, const Sky& sky);

    static void STDCALL ApiDebugCallback(
        uint32_t source,
        uint32_t type,
        uint32_t id,
        uint32_t severity,
        int32_t length,
        const char* message,
        const void* userParam);

    const uint8_t* m_VersionName;
    const uint8_t* m_RendererName;

    glm::vec3 m_ClearColor = glm::vec3(0.470f, 0.655f, 1.0f);

    // Opacity of deep water; the shader fades toward clear in the shallows.
    // Tunable live from the ImGui panel.
    float m_WaterOpacity = 0.95f;

    std::unique_ptr<ShaderProgram> m_Program;
    std::unique_ptr<Texture2D> m_Texture;

    std::unique_ptr<ShaderProgram> m_OutlineProgram;
    uint32_t m_OutlineVertexArray;
    uint32_t m_OutlineVertexBuffer;
    uint32_t m_OutlineElementBuffer;
    uint32_t m_OutlineElementCount;

    // Animated water: every 16x16 frame of the strip, stored RGBA back-to-back,
    // cycled into the atlas's water tile over time.
    std::vector<uint8_t> m_WaterFrames;
    int32_t m_WaterFrameCount = 0;
    int32_t m_WaterFrame = -1;
};

} // namespace Krafter
