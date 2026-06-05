#include "glad/gl.h"

#include "glm/gtc/matrix_transform.hpp"

#include "Krafter/Renderer/UIRenderer.h"
#include "Krafter/Window.h"

namespace Krafter {

UIRenderer::UIRenderer(Window& window)
    : m_Window(window)
    , m_Program("assets/ui.vert.glsl", "assets/ui.frag.glsl")
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
}

UIRenderer::~UIRenderer()
{
    glDeleteBuffers(1, &m_ElementBuffer);
    glDeleteBuffers(1, &m_VertexBuffer);
    glDeleteVertexArrays(1, &m_VertexArray);
}

void UIRenderer::Begin()
{
    glDisable(GL_DEPTH_TEST);

    m_Program.Bind();
    glBindVertexArray(m_VertexArray);

    // Top-left origin, y growing downward, in pixels.
    const glm::ivec2& size = m_Window.GetSize();
    glm::mat4 projection = glm::ortho(0.0f, (float)size.x, (float)size.y, 0.0f);
    m_Program.SetUniformMat4(0, projection);
}

void UIRenderer::End()
{
    glEnable(GL_DEPTH_TEST);
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

} // namespace Krafter
