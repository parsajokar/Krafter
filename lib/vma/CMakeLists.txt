cmake_minimum_required(VERSION 3.20)

project(vma LANGUAGES CXX)

find_package(Vulkan REQUIRED)

add_library(vma STATIC)

target_sources(
    vma
    PRIVATE
    include/vk_mem_alloc.h
    src/vma.cpp
)

# Expose the vendored VMA header (and the Vulkan headers it needs) to anything
# that links vma.
target_include_directories(vma PUBLIC include)

target_link_libraries(vma PUBLIC Vulkan::Vulkan)
