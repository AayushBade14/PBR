cmake_minimum_required(VERSION 3.10)
project(
  pbr
  VERSION 1.0
  DESCRIPTION "PBR-Renderer"
  LANGUAGES C CXX
)

include(FetchContent)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.3.9
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
  assimp
  GIT_REPOSITORY https://github.com/assimp/assimp.git
  GIT_TAG v5.4.3
)
FetchContent_MakeAvailable(assimp)

add_subdirectory(glm)

find_package(OpenGL REQUIRED)

add_library(
  glad
  STATIC
  ./glad/src/glad.c
)
target_include_directories(
  glad
  PUBLIC
  ./glad/include
)

add_library(
  stb_image
  STATIC
  ./stb_image.cpp
)
target_include_directories(
  stb_image
  PUBLIC
  ./
)

add_executable(
  pbr
  main.cpp
)
target_link_libraries(
  pbr
  PUBLIC
  OpenGL::GL
  glad
  glfw
  glm::glm
  assimp
  stb_image
)
