cmake_minimum_required(VERSION 3.20)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(krafter LANGUAGES CXX)

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

add_executable(krafter)

target_sources(
    krafter
    PRIVATE
    src/block.h
    src/block.cpp
    src/window.h
    src/window.cpp
    src/renderer.h
    src/renderer.cpp
    src/camera.h
    src/camera.cpp
    src/game.h
    src/game.cpp
    src/main.cpp
)

target_include_directories(
    krafter
    PRIVATE
    src
    lib/glfw/include
    lib/glad/include
    lib/imgui
    lib/imgui/backends
    lib/glm
    lib/stb/include
    lib/FastNoiseLite
)

target_link_libraries(
    krafter
    PRIVATE
    glfw
    glad
    imgui
    glm
    stb
    ${OPENGL_gl_LIBRARY}
)
