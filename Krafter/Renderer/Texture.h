#pragma once

#include <cstdint>
#include <string_view>

#include "glm/glm.hpp"

namespace Krafter {

class Texture2D {
public:
    Texture2D(std::string_view path);
    ~Texture2D();

    void Bind(uint32_t unit) const;

    inline const glm::ivec2& GetSize() const
    {
        return m_Size;
    }

private:
    uint32_t m_Id;
    glm::ivec2 m_Size;
};

} // namespace Krafter
