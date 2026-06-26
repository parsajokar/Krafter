#pragma once

#include "glm/glm.hpp"

#include "Krafter/Core/Window.h"

namespace Krafter {

enum class EventType {
    k_KeyPressed,
    k_KeyReleased,
    k_TextInput,
    k_MouseButtonPressed,
    k_MouseButtonReleased,
    k_MouseMoved,
    k_MouseScrolled,
    k_WindowResized,
};

// Which fields are meaningful depends on `type`.
struct Event {
    EventType type;
    bool handled = false;

    Key key = {};
    bool isRepeat = false;

    // A typed Unicode code point (k_TextInput), for text fields.
    unsigned int codepoint = 0;

    MouseButton button = {};

    // Cursor position (k_MouseMoved) or scroll offset (k_MouseScrolled).
    glm::vec2 mouse = {};

    glm::ivec2 size = {};
};

inline bool IsMouseEvent(EventType type)
{
    return type == EventType::k_MouseButtonPressed
        || type == EventType::k_MouseButtonReleased
        || type == EventType::k_MouseMoved
        || type == EventType::k_MouseScrolled;
}

inline bool IsKeyEvent(EventType type)
{
    return type == EventType::k_KeyPressed || type == EventType::k_KeyReleased
        || type == EventType::k_TextInput;
}

} // namespace Krafter
