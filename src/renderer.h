#pragma once

#include <string>
#include <string_view>
#include <array>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtx/hash.hpp"

#include "block.h"
#include "camera.h"

namespace Krafter
{

class Texture2D
{
public:
    Texture2D(std::string_view path);
    ~Texture2D();

    void Bind(uint32_t unit) const;

    inline const glm::ivec2& GetSize() const
    {
        return _size;
    }

private:
    uint32_t _id;
    glm::ivec2 _size;
};

class ShaderProgram
{
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

    uint32_t _id;
};

enum class BlockFace
{
    FRONT,
    BACK,
    LEFT,
    RIGHT,
    BOTTOM,
    TOP
};

class ChunkMesh
{
public:
    ChunkMesh(const ChunkMap& chunkMap, const glm::ivec2& chunkPosition);
    ~ChunkMesh();

    inline uint32_t GetElementCount() const
    {
        return _elementCount;
    }

    void Bind() const;

private:
    static void AddFaceToData(
        const std::array<glm::vec3, 4>& positionList,
        const std::array<glm::vec2, 2>& uvCoordsList,
        std::vector<float>& vertexBufferData, std::vector<uint32_t>& elementBufferData);
    static void AddFaceToData(
        const glm::vec3& position,
        Block block, BlockFace face,
        std::vector<float>& vertexBufferData, std::vector<uint32_t>& elementBufferData);

    uint32_t _elementCount;

    uint32_t _vertexArray;
    uint32_t _vertexBuffer;
    uint32_t _elementBuffer;
};

class Renderer
{
public:
    static void Init();
    static void Deinit();
    inline static Renderer* Get()
    { 
        return _instance;
    }

    inline Camera& GetCamera()
    {
        return _camera;
    }

    void GenerateAllChunkMeshes(const ChunkMap& chunkMap);
    void RegenerateChunkMesh(const ChunkMap& chunkMap, const glm::ivec2& chunkPosition);
    void DeleteChunkMesh(const glm::ivec2& chunkPosition);

    void ClearBuffers() const;
    void RenderChunkMeshes() const;
    void RenderImGui();

private:
    static void ApiDebugCallback(
        uint32_t source,
        uint32_t type,
        uint32_t id,
        uint32_t severity,
        int32_t length,
        const char* message,
        const void* userParam);

    inline static Renderer* _instance;

    Renderer();
    ~Renderer();

    const uint8_t* _versionName;
    const uint8_t* _rendererName;

    Camera _camera;

    std::shared_ptr<ShaderProgram> _program;
    std::shared_ptr<Texture2D> _texture;
    std::unordered_map<glm::ivec2, std::shared_ptr<ChunkMesh>> _chunkMeshes;
};

} // namespace Krafter
