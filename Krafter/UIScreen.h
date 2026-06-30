#pragma once

#include <string_view>

#include "glm/glm.hpp"

#include "Krafter/Core/Layer.h"

namespace Krafter {

class Window;
class UIRenderer;
class Texture2D;
class Font;

// Common base for the UI layers (the HUD and the full-screen menus). They all
// borrow the application-owned window, UI renderer, sprite sheet, and font, and
// the interactive ones track the cursor; this gathers those shared members so
// each layer only declares what's unique to it. Drawing and input stay each
// layer's own (OnRender/OnEvent), since those differ.
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

    // Last known cursor position, for the layers that hit-test against it (the
    // menus and inventory). The HUD leaves it unused.
    glm::vec2 m_Cursor = glm::vec2(0.0f);
};

} // namespace Krafter
