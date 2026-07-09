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

set(IMGUI_GLFW_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/glfw/include")
add_subdirectory(lib/imgui)

add_subdirectory(lib/glm)

add_subdirectory(lib/stb)

add_subdirectory(lib/vma)

add_subdirectory(lib/FastNoiseLite)

find_package(Vulkan REQUIRED)

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
    Krafter/Renderer/Pipeline.h
    Krafter/Renderer/Pipeline.cpp
    Krafter/Renderer/WorldRenderer.h
    Krafter/Renderer/WorldRenderer.cpp
    Krafter/Renderer/UIRenderer.h
    Krafter/Renderer/UIRenderer.cpp
    Krafter/Renderer/Widgets.h
    Krafter/Renderer/Widgets.cpp
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
    Krafter/InventoryLayer.h
    Krafter/InventoryLayer.cpp
    Krafter/Hotbar.h
    Krafter/Inventory.h
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
    imgui
    glm
    stb
    vma
    FastNoiseLite
    Vulkan::Vulkan
)

target_compile_definitions(
    Krafter
    PRIVATE
    GLM_ENABLE_EXPERIMENTAL
    # Vulkan clip space uses a [0, 1] depth range (not OpenGL's [-1, 1]), so GLM's
    # projection matrices must match. The Y-flip is handled by a negative-height
    # viewport in the renderer.
    GLM_FORCE_DEPTH_ZERO_TO_ONE
)

# Shaders: compile each GLSL source to SPIR-V with glslc (from the Vulkan SDK,
# located by find_package(Vulkan)). The .spv is written next to its source so the
# app loads it by swapping the extension. The stage is taken from the .vert/.frag
# infix, since glslc can't infer it from the .glsl extension.
set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders)
file(GLOB SHADER_SOURCES ${SHADER_DIR}/*.vert.glsl ${SHADER_DIR}/*.frag.glsl)

set(SPIRV_OUTPUTS "")
foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    string(REGEX REPLACE "\\.glsl$" ".spv" SPIRV_NAME ${SHADER_NAME})
    set(SPIRV_FILE ${SHADER_DIR}/${SPIRV_NAME})
    if(SHADER_NAME MATCHES "\\.vert\\.glsl$")
        set(SHADER_STAGE vertex)
    else()
        set(SHADER_STAGE fragment)
    endif()
    add_custom_command(
        OUTPUT ${SPIRV_FILE}
        COMMAND ${Vulkan_GLSLC_EXECUTABLE} -fshader-stage=${SHADER_STAGE} ${SHADER} -o ${SPIRV_FILE}
        DEPENDS ${SHADER}
        COMMENT "Compiling shader ${SHADER_NAME} -> ${SPIRV_NAME}"
        VERBATIM
    )
    list(APPEND SPIRV_OUTPUTS ${SPIRV_FILE})
endforeach()

add_custom_target(Shaders DEPENDS ${SPIRV_OUTPUTS})
add_dependencies(Krafter Shaders)
