#include <utility>
#include <fstream>
#include <iostream>

#include "glad/gl.h"
#include "GLFW/glfw3.h"
#include "imgui.h"
#include "glm/gtc/type_ptr.hpp"
#include "stb_image.h"

#include "renderer.h"

namespace Krafter
{

Texture2D::Texture2D(std::string_view path)
{
    stbi_set_flip_vertically_on_load(true);

    int32_t channels_in_file;
    uint8_t* data = stbi_load(path.data(), &_size.x, &_size.y, &channels_in_file, 0);
    if (!data) {
        std::cerr << "[FILE] Could not read " << path << std::endl;
        assert(false);
    }

    glCreateTextures(GL_TEXTURE_2D, 1, &_id);

    glTextureParameteri(_id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(_id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTextureStorage2D(_id, 1, GL_RGBA8, _size.x, _size.y);
    glTextureSubImage2D(_id, 0, 0, 0, _size.x, _size.y, GL_RGBA, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
}

Texture2D::~Texture2D()
{
    glDeleteTextures(1, &_id);
}

void Texture2D::Bind(uint32_t unit) const
{
    glBindTextureUnit(unit, _id);
}

ShaderProgram::ShaderProgram(std::string_view vertexShaderPath, std::string_view fragmentShaderPath)
{
    std::string vertexShaderSource = std::move(ReadFileAsString(vertexShaderPath));
    std::string fragmentShaderSource = std::move(ReadFileAsString(fragmentShaderPath));

    _id = glCreateProgram();
    uint32_t vertexShader = CreateShader(GL_VERTEX_SHADER, vertexShaderSource.c_str());
    uint32_t fragmentShader = CreateShader(GL_FRAGMENT_SHADER, fragmentShaderSource.c_str());

    glAttachShader(_id, vertexShader);
    glAttachShader(_id, fragmentShader);

    glLinkProgram(_id);
    glValidateProgram(_id);

    glDetachShader(_id, vertexShader);
    glDetachShader(_id, fragmentShader);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

ShaderProgram::~ShaderProgram()
{
    glDeleteProgram(_id);
}

void ShaderProgram::Bind() const
{
    glUseProgram(_id);
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

ChunkMesh::ChunkMesh(const ChunkMap& chunkMap, const glm::ivec2& chunkPosition)
{
    std::vector<float> vertexBufferData;
    std::vector<uint32_t> elementBufferData;

    constexpr int32_t dx[] = { -1, 1, 0, 0, 0, 0 };
    constexpr int32_t dy[] = { 0, 0, -1, 1, 0, 0 };
    constexpr int32_t dz[] = { 0, 0, 0, 0, -1, 1 };
    constexpr BlockFace faces[] = {
        BlockFace::FRONT, BlockFace::BACK,
        BlockFace::BOTTOM, BlockFace::TOP,
        BlockFace::LEFT, BlockFace::RIGHT
    };

    for (int32_t x = 0; x < Chunk::WIDTH; x++) {
        for (int32_t y = 0; y < Chunk::HEIGHT; y++) {
            for (int32_t z = 0; z < Chunk::WIDTH; z++) {
                if (chunkMap.at(chunkPosition).GetBlock(glm::ivec3(x, y, z)) == Block::AIR) {
                    continue;
                }

                for (size_t k = 0; k < 6; k++) {
                    int32_t nx = x + dx[k];
                    int32_t ny = y + dy[k];
                    int32_t nz = z + dz[k];
                    BlockFace face = faces[k];

                    if (nx < 0 || nx >= Chunk::WIDTH ||
                        ny < 0 || ny >= Chunk::HEIGHT ||
                        nz < 0 || nz >= Chunk::WIDTH ||
                        chunkMap.at(chunkPosition).GetBlock(glm::ivec3(nx, ny, nz)) == Block::AIR) {
                        AddFaceToData(
                            glm::vec3(
                                chunkMap.at(chunkPosition).GetPosition().x * Chunk::WIDTH + x,
                                y,
                                chunkMap.at(chunkPosition).GetPosition().y * Chunk::WIDTH + z),
                            chunkMap.at(chunkPosition).GetBlock(glm::ivec3(x, y, z)), face,
                            vertexBufferData, elementBufferData);
                    }
                }
            }
        }
    }

    _elementCount = elementBufferData.size();

    glCreateVertexArrays(1, &_vertexArray);
    glCreateBuffers(1, &_vertexBuffer);
    glCreateBuffers(1, &_elementBuffer);

    glNamedBufferData(_vertexBuffer, vertexBufferData.size() * sizeof(float), vertexBufferData.data(), GL_STATIC_DRAW);
    glNamedBufferData(_elementBuffer, elementBufferData.size() * sizeof(uint32_t), elementBufferData.data(), GL_STATIC_DRAW);

    glVertexArrayVertexBuffer(_vertexArray, 0, _vertexBuffer, 0, 5 * sizeof(float));
    glVertexArrayElementBuffer(_vertexArray, _elementBuffer);

    glEnableVertexArrayAttrib(_vertexArray, 0);
    glVertexArrayAttribBinding(_vertexArray, 0, 0);
    glVertexArrayAttribFormat(_vertexArray, 0, 3, GL_FLOAT, GL_FALSE, 0);

    glEnableVertexArrayAttrib(_vertexArray, 1);
    glVertexArrayAttribBinding(_vertexArray, 1, 0);
    glVertexArrayAttribFormat(_vertexArray, 1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
}

ChunkMesh::~ChunkMesh()
{
    glDeleteBuffers(1, &_elementBuffer);
    glDeleteBuffers(1, &_vertexBuffer);
    glDeleteVertexArrays(1, &_vertexArray);
}

void ChunkMesh::Bind() const
{
    glBindVertexArray(_vertexArray);
}

void ChunkMesh::AddFaceToData(
    const std::array<glm::vec3, 4>& positionList,
    const std::array<glm::vec2, 2>& uvCoordsList,
    std::vector<float>& vertexBufferData, std::vector<uint32_t>& elementBufferData)
{
    const size_t offset = vertexBufferData.size() / 5;

    vertexBufferData.push_back(positionList[0].x);
    vertexBufferData.push_back(positionList[0].y);
    vertexBufferData.push_back(positionList[0].z);
    vertexBufferData.push_back(uvCoordsList[0].x);
    vertexBufferData.push_back(uvCoordsList[0].y);

    vertexBufferData.push_back(positionList[1].x);
    vertexBufferData.push_back(positionList[1].y);
    vertexBufferData.push_back(positionList[1].z);
    vertexBufferData.push_back(uvCoordsList[1].x);
    vertexBufferData.push_back(uvCoordsList[0].y);

    vertexBufferData.push_back(positionList[2].x);
    vertexBufferData.push_back(positionList[2].y);
    vertexBufferData.push_back(positionList[2].z);
    vertexBufferData.push_back(uvCoordsList[1].x);
    vertexBufferData.push_back(uvCoordsList[1].y);

    vertexBufferData.push_back(positionList[3].x);
    vertexBufferData.push_back(positionList[3].y);
    vertexBufferData.push_back(positionList[3].z);
    vertexBufferData.push_back(uvCoordsList[0].x);
    vertexBufferData.push_back(uvCoordsList[1].y);

    elementBufferData.push_back(offset);
    elementBufferData.push_back(offset + 2);
    elementBufferData.push_back(offset + 1);

    elementBufferData.push_back(offset);
    elementBufferData.push_back(offset + 2);
    elementBufferData.push_back(offset + 3);
}

void ChunkMesh::AddFaceToData(
    const glm::vec3& position,
    const Block block, BlockFace face,
    std::vector<float>& vertexBufferData, std::vector<uint32_t>& elementBufferData)
{
    std::array<glm::vec3, 4> positionList;
    std::array<glm::vec2, 2> uvCoordsList;

    glm::vec3 origin;
    glm::vec3 dx;
    glm::vec3 dy;

    const BlockAtlas& atlas = BlockAtlas::GetAtlasOf(block);

    switch (face) {
    case BlockFace::FRONT:
        origin = position;
        dx = glm::vec3(0.0f, 0.0f, 1.0f);
        dy = glm::vec3(0.0f, 1.0f, 0.0f);
        uvCoordsList[0] = atlas.side;
        uvCoordsList[1] = atlas.side + glm::vec2(BlockAtlas::STEP, BlockAtlas::STEP);
        break;

    case BlockFace::BACK:
        origin = position + glm::vec3(1.0f, 0.0f, 1.0f);
        dx = glm::vec3(0.0f, 0.0f, -1.0f);
        dy = glm::vec3(0.0f, 1.0f, 0.0f);
        uvCoordsList[0] = atlas.side;
        uvCoordsList[1] = atlas.side + glm::vec2(BlockAtlas::STEP, BlockAtlas::STEP);
        break;

    case BlockFace::LEFT:
        origin = position + glm::vec3(1.0f, 0.0f, 0.0f);
        dx = glm::vec3(-1.0f, 0.0f, 0.0f);
        dy = glm::vec3(0.0f, 1.0f, 0.0f);
        uvCoordsList[0] = atlas.side;
        uvCoordsList[1] = atlas.side + glm::vec2(BlockAtlas::STEP, BlockAtlas::STEP);
        break;

    case BlockFace::RIGHT:
        origin = position + glm::vec3(0.0f, 0.0f, 1.0f);
        dx = glm::vec3(1.0f, 0.0f, 0.0f);
        dy = glm::vec3(0.0f, 1.0f, 0.0f);
        uvCoordsList[0] = atlas.side;
        uvCoordsList[1] = atlas.side + glm::vec2(BlockAtlas::STEP, BlockAtlas::STEP);
        break;

    case BlockFace::BOTTOM:
        origin = position + glm::vec3(1.0f, 0.0f, 0.0f);
        dx = glm::vec3(0.0f, 0.0f, 1.0f);
        dy = glm::vec3(-1.0f, 0.0f, 0.0f);
        uvCoordsList[0] = atlas.bottom;
        uvCoordsList[1] = atlas.bottom + glm::vec2(BlockAtlas::STEP, BlockAtlas::STEP);
        break;

    default: // BlockFace::TOP
        origin = position + glm::vec3(0.0f, 1.0f, 0.0f);
        dx = glm::vec3(0.0f, 0.0f, 1.0f);
        dy = glm::vec3(1.0f, 0.0f, 0.0f);
        uvCoordsList[0] = atlas.top;
        uvCoordsList[1] = atlas.top + glm::vec2(BlockAtlas::STEP, BlockAtlas::STEP);
        break;
    }

    positionList[0] = origin;
    positionList[1] = origin + dx;
    positionList[2] = origin + dx + dy;
    positionList[3] = origin + dy;

    AddFaceToData(positionList, uvCoordsList, vertexBufferData, elementBufferData);
}

void Renderer::Init()
{
    _instance = new Renderer();
}

void Renderer::Deinit()
{
    delete _instance;
}

void Renderer::GenerateAllChunkMeshes(const ChunkMap& chunkMap)
{
    _chunkMeshes.clear();
    for (const auto& [chunkPosition, chunk] : chunkMap) {
        _chunkMeshes[chunkPosition] = std::make_shared<ChunkMesh>(chunkMap, chunkPosition);
    }
}

void Renderer::RegenerateChunkMesh(const ChunkMap& chunkMap, const glm::ivec2& chunkPosition)
{
    _chunkMeshes[chunkPosition] = std::make_shared<ChunkMesh>(chunkMap, chunkPosition);
}

void Renderer::ClearBuffers() const
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::RenderChunkMeshes() const
{
    _texture->Bind(0);
    _program->Bind();
    _program->SetUniformMat4(0, _camera.GetViewProjection());
    _program->SetUniformInt(1, 0);
    for (const auto& [position, chunkMesh] : _chunkMeshes) {
        chunkMesh->Bind();
        glDrawElements(GL_TRIANGLES, chunkMesh->GetElementCount(), GL_UNSIGNED_INT, nullptr);
    }
}

void Renderer::RenderImGui()
{
    ImGui::Text("OpenGL Details:");
    ImGui::Text("Version: %s", _versionName);
    ImGui::Text("Renderer: %s", _rendererName);

    ImGui::Separator();

    _camera.RenderImGui();

    ImGui::Separator();
}

Renderer::Renderer()
    : _camera(glm::vec3(0.0f), glm::radians(80.0f))
{
    gladLoadGL(glfwGetProcAddress);

    _versionName = glGetString(GL_VERSION);
    _rendererName = glGetString(GL_RENDERER);

    glDebugMessageCallback(ApiDebugCallback, nullptr);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    BlockAtlas::LoadAtlases();

    _program = std::make_shared<ShaderProgram>("assets/default.vert.glsl", "assets/default.frag.glsl");
    _texture = std::make_shared<Texture2D>("assets/texture.png");
}

Renderer::~Renderer()
{
}

void Renderer::ApiDebugCallback(
        uint32_t source,
        uint32_t type,
        uint32_t id,
        uint32_t severity,
        int32_t length,
        const char* message,
        const void* userParam)
{
    std::cerr << "[OPENGL] " << message << std::endl;
    assert(severity != GL_DEBUG_SEVERITY_HIGH);
}

} // namespace Krafter
