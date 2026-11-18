cmake_minimum_required(VERSION 3.22)
project(GameEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Put the .exe in build/bin so it is easy to find
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Download and build SFML automatically (needs git + internet on first configure)
include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.1
    GIT_SHALLOW ON)
# Only the graphics module is needed - skip audio/network to speed up builds
set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SFML)

add_executable(GameEngine
    src/main.cpp
    src/Engine.cpp
    src/Logger.cpp
    src/PrimitiveRenderer.cpp
    src/Point2D.cpp
    src/LineSegment.cpp
    src/LineComparison.cpp
    src/RasterTests.cpp
    src/TransformableObject.cpp
    src/Shapes.cpp
    src/Player.cpp
    src/BitmapHandler.cpp
    src/BitmapObject.cpp
    src/SpriteObject.cpp
    src/DemoAssets.cpp)

target_include_directories(GameEngine PRIVATE src)
target_link_libraries(GameEngine PRIVATE SFML::Graphics)
