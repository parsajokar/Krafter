cmake_minimum_required(VERSION 3.20)

project(FastNoiseLite LANGUAGES CXX)

add_library(FastNoiseLite STATIC)

target_sources(
    FastNoiseLite
    PRIVATE
    FastNoiseLite.h
    FastNoiseLite.cpp
)
