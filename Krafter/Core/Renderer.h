#pragma once

#include <cstdint>

#include "glm/glm.hpp"

#include "Krafter/Core/Platform.h"

namespace Krafter {

// The generic graphics device: owns the GL context setup and the per-frame
// framebuffer clear that every screen needs. It knows nothing about what is
// being drawn; concrete renderers (WorldRenderer, UIRenderer) issue the actual
// draw calls.
class Renderer {
public:
    Renderer();
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void SetClearColor(const glm::vec3& color);
    void Clear();

    void RenderImGui();

private:
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
};

} // namespace Krafter
