#include "glad/gl.h"

#include "glm/gtc/matrix_transform.hpp"

#include "Krafter/Renderer/UIRenderer.h"
#include "Krafter/Core/Window.h"

namespace Krafter {

UIRenderer::UIRenderer(Window& window)
    : m_Window(window)
    , m_Program("assets/shaders/ui.vert.glsl", "assets/shaders/ui.frag.glsl")
{
    // Unit quad: (0,0) top-left to (1,1) bottom-right, with matching UVs.
    const float vertices[] = {
        0.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f
    };
    const uint32_t elements[] = { 0, 1, 2, 0, 2, 3 };

    glCreateVertexArrays(1, &m_VertexArray);
    glCreateBuffers(1, &m_VertexBuffer);
    glCreateBuffers(1, &m_ElementBuffer);

    glNamedBufferData(m_VertexBuffer, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glNamedBufferData(m_ElementBuffer, sizeof(elements), elements, GL_STATIC_DRAW);

    glVertexArrayVertexBuffer(m_VertexArray, 0, m_VertexBuffer, 0, 4 * sizeof(float));
    glVertexArrayElementBuffer(m_VertexArray, m_ElementBuffer);

    glEnableVertexArrayAttrib(m_VertexArray, 0);
    glVertexArrayAttribBinding(m_VertexArray, 0, 0);
    glVertexArrayAttribFormat(m_VertexArray, 0, 2, GL_FLOAT, GL_FALSE, 0);

    glEnableVertexArrayAttrib(m_VertexArray, 1);
    glVertexArrayAttribBinding(m_VertexArray, 1, 0);
    glVertexArrayAttribFormat(m_VertexArray, 1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float));

    // Dynamic quad: same vertex layout and element buffer, but its four vertices
    // are re-uploaded on each draw.
    glCreateVertexArrays(1, &m_PolyVertexArray);
    glCreateBuffers(1, &m_PolyVertexBuffer);

    glNamedBufferData(m_PolyVertexBuffer, 4 * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

    glVertexArrayVertexBuffer(m_PolyVertexArray, 0, m_PolyVertexBuffer, 0, 4 * sizeof(float));
    glVertexArrayElementBuffer(m_PolyVertexArray, m_ElementBuffer);

    glEnableVertexArrayAttrib(m_PolyVertexArray, 0);
    glVertexArrayAttribBinding(m_PolyVertexArray, 0, 0);
    glVertexArrayAttribFormat(m_PolyVertexArray, 0, 2, GL_FLOAT, GL_FALSE, 0);

    glEnableVertexArrayAttrib(m_PolyVertexArray, 1);
    glVertexArrayAttribBinding(m_PolyVertexArray, 1, 0);
    glVertexArrayAttribFormat(m_PolyVertexArray, 1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float));
}

UIRenderer::~UIRenderer()
{
    glDeleteBuffers(1, &m_PolyVertexBuffer);
    glDeleteVertexArrays(1, &m_PolyVertexArray);
    glDeleteBuffers(1, &m_ElementBuffer);
    glDeleteBuffers(1, &m_VertexBuffer);
    glDeleteVertexArrays(1, &m_VertexArray);
}

void UIRenderer::Begin()
{
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_Program.Bind();
    glBindVertexArray(m_VertexArray);

    // Top-left origin, y growing downward, in pixels.
    const glm::ivec2& size = m_Window.GetSize();
    glm::mat4 projection = glm::ortho(0.0f, (float)size.x, (float)size.y, 0.0f);
    m_Program.SetUniformMat4(0, projection);
}

void UIRenderer::End()
{
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void UIRenderer::SetScissor(const glm::vec2& position, const glm::vec2& size)
{
    // glScissor is measured from the bottom-left, but UI space is top-left, so
    // flip the y against the framebuffer height.
    const glm::ivec2& windowSize = m_Window.GetSize();
    const int32_t y = windowSize.y - static_cast<int32_t>(position.y + size.y);

    glEnable(GL_SCISSOR_TEST);
    glScissor(static_cast<int32_t>(position.x), y,
        static_cast<int32_t>(size.x), static_cast<int32_t>(size.y));
}

void UIRenderer::ClearScissor()
{
    glDisable(GL_SCISSOR_TEST);
}

void UIRenderer::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
{
    DrawQuad(position, size, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f), color, nullptr);
}

void UIRenderer::DrawQuad(
    const glm::vec2& position, const glm::vec2& size, const Texture2D& texture,
    const glm::vec4& uvRect, const glm::vec4& tint)
{
    DrawQuad(position, size, uvRect, tint, &texture);
}

void UIRenderer::DrawQuad(
    const glm::vec2& position, const glm::vec2& size,
    const glm::vec4& uvRect, const glm::vec4& tint, const Texture2D* texture)
{
    glBindVertexArray(m_VertexArray);

    m_Program.SetUniformVec4(1, glm::vec4(position, size));
    m_Program.SetUniformVec4(2, uvRect);
    m_Program.SetUniformVec4(4, tint);

    if (texture) {
        texture->Bind(0);
        m_Program.SetUniformInt(3, 0);
        m_Program.SetUniformInt(5, 1);
    } else {
        m_Program.SetUniformInt(5, 0);
    }

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

void UIRenderer::DrawQuadInverted(
    const glm::vec2& position, const glm::vec2& size, const Texture2D& texture,
    const glm::vec4& uvRect)
{
    glBindVertexArray(m_VertexArray);
    glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_COLOR);

    m_Program.SetUniformVec4(1, glm::vec4(position, size));
    m_Program.SetUniformVec4(2, uvRect);
    m_Program.SetUniformVec4(4, glm::vec4(1.0f));

    texture.Bind(0);
    m_Program.SetUniformInt(3, 0);
    m_Program.SetUniformInt(5, 1);
    m_Program.SetUniformInt(6, 1);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    // Restore the default UI blending and mode for following draws.
    m_Program.SetUniformInt(6, 0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void UIRenderer::DrawQuad(
    const std::array<glm::vec2, 4>& corners, const std::array<glm::vec2, 4>& uvs,
    const Texture2D& texture, const glm::vec4& tint)
{
    float vertices[4 * 4];
    for (int i = 0; i < 4; i++) {
        vertices[i * 4 + 0] = corners[i].x;
        vertices[i * 4 + 1] = corners[i].y;
        vertices[i * 4 + 2] = uvs[i].x;
        vertices[i * 4 + 3] = uvs[i].y;
    }
    glNamedBufferSubData(m_PolyVertexBuffer, 0, sizeof(vertices), vertices);

    glBindVertexArray(m_PolyVertexArray);

    // Corners and UVs are baked into the buffer, so the transform and uv-rect
    // pass through unchanged.
    m_Program.SetUniformVec4(1, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
    m_Program.SetUniformVec4(2, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
    m_Program.SetUniformVec4(4, tint);

    texture.Bind(0);
    m_Program.SetUniformInt(3, 0);
    m_Program.SetUniformInt(5, 1);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

} // namespace Krafter
