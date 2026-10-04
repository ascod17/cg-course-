cmake_minimum_required(VERSION 3.16)
project(cg LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# --- GLFW (автоматты жүктеледі, интернет керек) ---
include(FetchContent)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4)
FetchContent_MakeAvailable(glfw)

# --- GLAD (external/glad ішінде дайын тұр) ---
add_library(glad STATIC external/glad/src/gl.c)
target_include_directories(glad PUBLIC external/glad/include)

# --- Негізгі бағдарлама ---
add_executable(cg main.cpp)
target_link_libraries(cg PRIVATE glad glfw)
