#pragma once

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

private:
    void DrawQuad(
        const glm::vec2& position, const glm::vec2& size,
        const glm::vec4& uvRect, const glm::vec4& tint, const Texture2D* texture);

    Window& m_Window;

    ShaderProgram m_Program;

    uint32_t m_VertexArray;
    uint32_t m_VertexBuffer;
    uint32_t m_ElementBuffer;
};

} // namespace Krafter
