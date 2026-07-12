#pragma once

#include <string_view>

#include "glm/glm.hpp"

#include "Krafter/Core/Layer.h"

namespace Krafter {

class Window;
class UIRenderer;
class Texture2D;
class Font;

class UIScreen : public Layer {
protected:
    UIScreen(
        std::string_view name, Window& window, UIRenderer& renderer,
        Texture2D& uiTexture, Font& font)
        : Layer(name)
        , m_Window(window)
        , m_Renderer(renderer)
        , m_UITexture(uiTexture)
        , m_Font(font)
    {
    }

    Window& m_Window;
    UIRenderer& m_Renderer;
    Texture2D& m_UITexture;
    Font& m_Font;

    glm::vec2 m_Cursor = glm::vec2(0.0f);
};

}
