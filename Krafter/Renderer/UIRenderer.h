#pragma once

#include <array>
#include <cstdint>

#include "glm/glm.hpp"

#include "Krafter/Renderer/ShaderProgram.h"
#include "Krafter/Renderer/Texture.h"

namespace Krafter {

class Window;

// Draws quads in pixel space (origin top-left). Issue draws between Begin/End.
class UIRenderer {
public:
    UIRenderer(Window& window);
    ~UIRenderer();

    UIRenderer(const UIRenderer&) = delete;
    UIRenderer& operator=(const UIRenderer&) = delete;

    void Begin();
    void End();

    void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
    void DrawQuad(
        const glm::vec2& position, const glm::vec2& size, const Texture2D& texture,
        const glm::vec4& uvRect = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f),
        const glm::vec4& tint = glm::vec4(1.0f));

    // Draws a textured quad with four arbitrary corner positions (pixel space),
    // each paired with a UV. Corners are wound around the quad. Lets callers
    // build sheared shapes such as isometric block faces.
    void DrawQuad(
        const std::array<glm::vec2, 4>& corners, const std::array<glm::vec2, 4>& uvs,
        const Texture2D& texture, const glm::vec4& tint = glm::vec4(1.0f));

private:
    void DrawQuad(
        const glm::vec2& position, const glm::vec2& size,
        const glm::vec4& uvRect, const glm::vec4& tint, const Texture2D* texture);

    Window& m_Window;

    ShaderProgram m_Program;

    uint32_t m_VertexArray;
    uint32_t m_VertexBuffer;
    uint32_t m_ElementBuffer;

    // Separate dynamic buffer for arbitrary-corner quads, re-uploaded per draw.
    uint32_t m_PolyVertexArray;
    uint32_t m_PolyVertexBuffer;
};

} // namespace Krafter
