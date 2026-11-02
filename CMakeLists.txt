cmake_minimum_required(VERSION 3.20)
project(cnnx-the-game LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(raylib)

add_executable(cnnx
    src/main.cpp
    src/Player.cpp
    src/Room.cpp
)
target_link_libraries(cnnx PRIVATE raylib)
target_compile_options(cnnx PRIVATE -Wall -Wextra)

# Evita depender das DLLs do MinGW para rodar o .exe
if(MINGW)
    target_link_options(cnnx PRIVATE -static)
endif()
