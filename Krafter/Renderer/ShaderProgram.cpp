#include <fstream>
#include <iostream>
#include <string>

#include "glad/gl.h"

#include "glm/gtc/type_ptr.hpp"

#include "Krafter/Renderer/ShaderProgram.h"

namespace Krafter {

ShaderProgram::ShaderProgram(std::string_view vertexShaderPath, std::string_view fragmentShaderPath)
{
    std::string vertexShaderSource = std::move(ReadFileAsString(vertexShaderPath));
    std::string fragmentShaderSource = std::move(ReadFileAsString(fragmentShaderPath));

    m_id = glCreateProgram();
    uint32_t vertexShader = CreateShader(GL_VERTEX_SHADER, vertexShaderSource.c_str());
    uint32_t fragmentShader = CreateShader(GL_FRAGMENT_SHADER, fragmentShaderSource.c_str());

    glAttachShader(m_id, vertexShader);
    glAttachShader(m_id, fragmentShader);

    glLinkProgram(m_id);
    glValidateProgram(m_id);

    glDetachShader(m_id, vertexShader);
    glDetachShader(m_id, fragmentShader);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

ShaderProgram::~ShaderProgram()
{
    glDeleteProgram(m_id);
}

void ShaderProgram::Bind() const
{
    glUseProgram(m_id);
}

void ShaderProgram::SetUniformInt(int32_t location, int32_t value) const
{
    glUniform1i(location, value);
}

void ShaderProgram::SetUniformVec4(int32_t location, const glm::vec4& value) const
{
    glUniform4fv(location, 1, glm::value_ptr(value));
}

void ShaderProgram::SetUniformMat4(int32_t location, const glm::mat4& value) const
{
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

std::string ShaderProgram::ReadFileAsString(std::string_view path)
{
    std::ifstream file = std::ifstream(path.data(), std::ios::binary);
    if (!file) {
        std::cerr << "[FILE] Could not read " << path << std::endl;
        assert(false);
    }

    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();

    std::string result = std::string(size, '\0');
    file.seekg(0, std::ios::beg);
    file.read(result.data(), size);

    return result;
}

uint32_t ShaderProgram::CreateShader(uint32_t type, const char* source)
{
    uint32_t shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    return shader;
}

} // namespace Krafter
