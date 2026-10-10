cmake_minimum_required(VERSION 3.20)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(Krafter LANGUAGES CXX)

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

add_executable(Krafter)

target_sources(
    Krafter
    PRIVATE
    Krafter/Engine/Application.h
    Krafter/Engine/Application.cpp
    Krafter/Engine/Event.h
    Krafter/Engine/FixedTimestep.h
    Krafter/Engine/JobSystem.h
    Krafter/Engine/JobSystem.cpp
    Krafter/Engine/Layer.h
    Krafter/Engine/Layer.cpp
    Krafter/Engine/LayerStack.h
    Krafter/Engine/LayerStack.cpp
    Krafter/Engine/ResultQueue.h
    Krafter/Engine/Window.h
    Krafter/Engine/Window.cpp
    Krafter/Engine/Graphics/BufferManager.h
    Krafter/Engine/Graphics/BufferManager.cpp
    Krafter/Engine/Graphics/GeometryArena.h
    Krafter/Engine/Graphics/GeometryArena.cpp
    Krafter/Engine/Graphics/Image.h
    Krafter/Engine/Graphics/Image.cpp
    Krafter/Engine/Graphics/Pipeline.h
    Krafter/Engine/Graphics/Pipeline.cpp
    Krafter/Engine/Graphics/Renderer.h
    Krafter/Engine/Graphics/Renderer.cpp
    Krafter/Engine/Graphics/Swapchain.h
    Krafter/Engine/Graphics/Swapchain.cpp
    Krafter/Engine/Graphics/Texture.h
    Krafter/Engine/Graphics/Texture.cpp
    Krafter/Engine/Graphics/TextureManager.h
    Krafter/Engine/Graphics/TextureManager.cpp
    Krafter/Engine/Graphics/VulkanContext.h
    Krafter/Engine/Graphics/VulkanContext.cpp
    Krafter/Rendering/Camera.h
    Krafter/Rendering/Camera.cpp
    Krafter/Rendering/ChunkMesh.h
    Krafter/Rendering/ChunkMesh.cpp
    Krafter/Rendering/ChunkRenderer.h
    Krafter/Rendering/ChunkRenderer.cpp
    Krafter/Rendering/Font.h
    Krafter/Rendering/Font.cpp
    Krafter/Rendering/RenderView.h
    Krafter/Rendering/UIRenderer.h
    Krafter/Rendering/UIRenderer.cpp
    Krafter/Rendering/WorldRenderer.h
    Krafter/Rendering/WorldRenderer.cpp
    Krafter/World/Biome.h
    Krafter/World/Biome.cpp
    Krafter/World/Block.h
    Krafter/World/Block.cpp
    Krafter/World/BlockBehavior.h
    Krafter/World/BlockBehavior.cpp
    Krafter/World/Chunk.h
    Krafter/World/Chunk.cpp
    Krafter/World/Coords.h
    Krafter/World/DropSystem.h
    Krafter/World/DropSystem.cpp
    Krafter/World/FallingBlockSystem.h
    Krafter/World/FallingBlockSystem.cpp
    Krafter/World/Fluid.h
    Krafter/World/FluidSystem.h
    Krafter/World/FluidSystem.cpp
    Krafter/World/Item.h
    Krafter/World/Lighting.h
    Krafter/World/Lighting.cpp
    Krafter/World/Sky.h
    Krafter/World/Sky.cpp
    Krafter/World/TerrainGenerator.h
    Krafter/World/TerrainGenerator.cpp
    Krafter/World/World.h
    Krafter/World/World.cpp
    Krafter/World/WorldAccess.h
    Krafter/Gameplay/Crafting.h
    Krafter/Gameplay/Crafting.cpp
    Krafter/Gameplay/GameMode.h
    Krafter/Gameplay/Hotbar.h
    Krafter/Gameplay/Inventory.h
    Krafter/Gameplay/ItemContainer.h
    Krafter/Gameplay/ItemContainer.cpp
    Krafter/Gameplay/Player.h
    Krafter/Gameplay/Player.cpp
    Krafter/UI/HudLayer.h
    Krafter/UI/HudLayer.cpp
    Krafter/UI/InventoryLayer.h
    Krafter/UI/InventoryLayer.cpp
    Krafter/UI/MainMenuLayer.h
    Krafter/UI/MainMenuLayer.cpp
    Krafter/UI/PauseMenuLayer.h
    Krafter/UI/PauseMenuLayer.cpp
    Krafter/UI/UIScreen.h
    Krafter/UI/Widgets.h
    Krafter/UI/Widgets.cpp
    Krafter/Game/GameApplication.h
    Krafter/Game/GameApplication.cpp
    Krafter/Game/WorldLayer.h
    Krafter/Game/WorldLayer.cpp
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
    GLM_FORCE_DEPTH_ZERO_TO_ONE
)

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
