cmake_minimum_required(VERSION 3.20)
project(cnnx-the-game LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(CNNX_DEV_KEYS "Ativa F1/F2/F3 para trocar a forca do bloco" ON)

include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(raylib nlohmann_json)

# Regras do jogo, sem raylib, para rodar tambem nos testes sem janela
add_library(cnnx_core STATIC
    src/Enemy.cpp
    src/Game.cpp
    src/Hazards.cpp
    src/Items.cpp
    src/LevelLoader.cpp
    src/Player.cpp
    src/Profile.cpp
    src/Room.cpp
    src/TitleMenu.cpp
    src/WirePuzzle.cpp
)
target_include_directories(cnnx_core PUBLIC src)
target_link_libraries(cnnx_core PUBLIC nlohmann_json::nlohmann_json)
target_compile_options(cnnx_core PRIVATE -Wall -Wextra)

add_executable(cnnx
    src/GameRender.cpp
    src/main.cpp
)
target_link_libraries(cnnx PRIVATE cnnx_core raylib)
target_compile_options(cnnx PRIVATE -Wall -Wextra)
if(CNNX_DEV_KEYS)
    target_compile_definitions(cnnx PRIVATE CNNX_DEV_KEYS)
endif()

add_custom_command(TARGET cnnx POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:cnnx>/assets
)

# Evita depender das DLLs do MinGW para rodar o .exe
if(MINGW)
    target_link_options(cnnx PRIVATE -static)
endif()

enable_testing()
add_executable(cnnx_tests
    tests/test_game.cpp
    tests/test_main.cpp
    tests/test_menu.cpp
    tests/test_player.cpp
)
target_link_libraries(cnnx_tests PRIVATE cnnx_core)
target_compile_options(cnnx_tests PRIVATE -Wall -Wextra)
target_compile_definitions(cnnx_tests PRIVATE CNNX_LEVEL_PATH="${CMAKE_SOURCE_DIR}/assets/levels/cnnx.ldtk")
if(MINGW)
    target_link_options(cnnx_tests PRIVATE -static)
endif()
add_test(NAME cnnx_tests COMMAND cnnx_tests)
