cmake_minimum_required(VERSION 3.20)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(Krafter LANGUAGES CXX)

# Libraries

set(CMAKE_FOLDER "Libraries")

set(GLFW_BUILD_DOCS OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
add_subdirectory(lib/glfw)
set_target_properties(glfw PROPERTIES FOLDER "Libraries/GLFW")
set_target_properties(uninstall PROPERTIES FOLDER "Libraries/GLFW")
set_target_properties(update_mappings PROPERTIES FOLDER "Libraries/GLFW")

add_subdirectory(lib/glad)

set(IMGUI_GLFW_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/glfw/include")
add_subdirectory(lib/imgui)

add_subdirectory(lib/glm)

add_subdirectory(lib/stb)

add_subdirectory(lib/FastNoiseLite)

find_package(OpenGL REQUIRED)

unset(CMAKE_FOLDER)

# Krafter

add_executable(Krafter)

target_sources(
    Krafter
    PRIVATE
    Krafter/World/ChunkManager.h
    Krafter/World/ChunkManager.cpp
    Krafter/World/Chunk.h
    Krafter/World/Chunk.cpp
    Krafter/World/Block.h
    Krafter/World/Block.cpp
    Krafter/Renderer/ChunkMesh.h
    Krafter/Renderer/ChunkMesh.cpp
    Krafter/Renderer/Texture.h
    Krafter/Renderer/Texture.cpp
    Krafter/Renderer/ShaderProgram.h
    Krafter/Renderer/ShaderProgram.cpp
    Krafter/Renderer/Renderer.h
    Krafter/Renderer/Renderer.cpp
    Krafter/Game.h
    Krafter/Game.cpp
    Krafter/Camera.h
    Krafter/Camera.cpp
    Krafter/Window.h
    Krafter/Window.cpp
    Krafter/LayerStack.h
    Krafter/LayerStack.cpp
    Krafter/Layer.h
    Krafter/Layer.cpp
    Krafter/Application.h
    Krafter/Application.cpp
    Krafter/Main.cpp
)

target_include_directories(
    Krafter
    PRIVATE
    .
    lib/glfw/include
    lib/glad/include
    lib/imgui
    lib/imgui/backends
    lib/glm
    lib/stb/include
    lib/FastNoiseLite
)

target_link_libraries(
    Krafter
    PRIVATE
    glfw
    glad
    imgui
    glm
    stb
    FastNoiseLite
    ${OPENGL_gl_LIBRARY}
)

target_compile_definitions(
    Krafter
    PRIVATE
    GLM_ENABLE_EXPERIMENTAL
)
