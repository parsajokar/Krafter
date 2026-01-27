#pragma once

#include <cstdint>
#include <string_view>

#include "glm/glm.hpp"

namespace Krafter {

class ShaderProgram {
public:
    ShaderProgram(std::string_view vertexShaderPath, std::string_view fragmentShaderPath);
    ~ShaderProgram();

    void Bind() const;

    void SetUniformInt(int32_t location, int32_t value) const;
    void SetUniformVec4(int32_t location, const glm::vec4& value) const;
    void SetUniformMat4(int32_t location, const glm::mat4& value) const;

private:
    static std::string ReadFileAsString(std::string_view path);
    static uint32_t CreateShader(uint32_t type, const char* source);

    uint32_t m_id;
};

} // namespace Krafter
