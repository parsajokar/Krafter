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
    Krafter/World/World.h
    Krafter/World/World.cpp
    Krafter/World/Chunk.h
    Krafter/World/Chunk.cpp
    Krafter/World/Block.h
    Krafter/World/Block.cpp
    Krafter/World/Biome.h
    Krafter/World/Biome.cpp
    Krafter/World/Lighting.h
    Krafter/World/Lighting.cpp
    Krafter/World/Sky.h
    Krafter/World/Sky.cpp
    Krafter/World/Coords.h
    Krafter/Renderer/ChunkMesh.h
    Krafter/Renderer/ChunkMesh.cpp
    Krafter/Renderer/Texture.h
    Krafter/Renderer/Texture.cpp
    Krafter/Renderer/ShaderProgram.h
    Krafter/Renderer/ShaderProgram.cpp
    Krafter/Renderer/WorldRenderer.h
    Krafter/Renderer/WorldRenderer.cpp
    Krafter/Renderer/UIRenderer.h
    Krafter/Renderer/UIRenderer.cpp
    Krafter/Renderer/Font.h
    Krafter/Renderer/Font.cpp
    Krafter/WorldLayer.h
    Krafter/WorldLayer.cpp
    Krafter/UILayer.h
    Krafter/UILayer.cpp
    Krafter/MainMenuLayer.h
    Krafter/MainMenuLayer.cpp
    Krafter/PauseMenuLayer.h
    Krafter/PauseMenuLayer.cpp
    Krafter/Hotbar.h
    Krafter/GameMode.h
    Krafter/Game.h
    Krafter/Game.cpp
    Krafter/Camera.h
    Krafter/Camera.cpp
    Krafter/Player.h
    Krafter/Player.cpp
    Krafter/Core/Application.h
    Krafter/Core/Application.cpp
    Krafter/Core/Renderer.h
    Krafter/Core/Renderer.cpp
    Krafter/Core/Layer.h
    Krafter/Core/Layer.cpp
    Krafter/Core/LayerStack.h
    Krafter/Core/LayerStack.cpp
    Krafter/Core/Window.h
    Krafter/Core/Window.cpp
    Krafter/Core/Event.h
    Krafter/Core/JobSystem.h
    Krafter/Core/JobSystem.cpp
    Krafter/Core/ResultQueue.h
    Krafter/Core/Platform.h
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
