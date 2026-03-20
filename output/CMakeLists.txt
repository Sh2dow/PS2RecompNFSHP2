cmake_minimum_required(VERSION 3.21)
project(PS2RecompiledGame)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Get the project root directory (parent of output/)
get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)

# Find all generated source files in this directory
file(GLOB GENERATED_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# Add ps2xRuntime include directories from the parent project
include_directories("${PROJECT_ROOT}/ps2xRuntime/include")
include_directories("${PROJECT_ROOT}/ps2xRuntime/src/lib")

# Include generated headers in this directory after runtime headers so stale
# compatibility copies in output/ do not override the real runtime API.
include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

# Add fmt include directory from the build output
include_directories("${PROJECT_ROOT}/out/_deps/fmt-src/include")

# Add raylib include directory from the build output
include_directories("${PROJECT_ROOT}/out/_deps/raylib-src/src")

# Define the executable
add_executable(ps2_game ${GENERATED_SOURCES})

# MSVC-specific compile options for faster builds
if (MSVC)
    target_compile_options(ps2_game PRIVATE /MP /bigobj)
endif()

# Prefer linking directly against sibling targets when output/ is built through the
# top-level project. Standalone output/build still falls back to the known build-tree libs.
if (TARGET ps2_runtime)
    target_link_libraries(ps2_game PRIVATE
        ps2_runtime
        raylib
    )
else()
    target_link_libraries(ps2_game PRIVATE
        "$<$<CONFIG:Debug>:${PROJECT_ROOT}/out/build/ps2xRuntime/Debug/ps2_runtime.lib>"
        "$<$<CONFIG:Release>:${PROJECT_ROOT}/out/build/ps2xRuntime/Release/ps2_runtime.lib>"
        "$<$<CONFIG:MinSizeRel>:${PROJECT_ROOT}/out/build/ps2xRuntime/MinSizeRel/ps2_runtime.lib>"
        "$<$<CONFIG:RelWithDebInfo>:${PROJECT_ROOT}/out/build/ps2xRuntime/RelWithDebInfo/ps2_runtime.lib>"

        "$<$<CONFIG:Debug>:${PROJECT_ROOT}/out/build/_deps/raylib-build/raylib/Debug/raylib.lib>"
        "$<$<CONFIG:Release>:${PROJECT_ROOT}/out/build/_deps/raylib-build/raylib/Release/raylib.lib>"
        "$<$<CONFIG:MinSizeRel>:${PROJECT_ROOT}/out/build/_deps/raylib-build/raylib/MinSizeRel/raylib.lib>"
        "$<$<CONFIG:RelWithDebInfo>:${PROJECT_ROOT}/out/build/_deps/raylib-build/raylib/RelWithDebInfo/raylib.lib>"
    )
endif()

target_link_libraries(ps2_game PRIVATE
    winmm.lib
    odbc32.lib
    odbccp32.lib
)

# Set the output directory for the executable
set_target_properties(ps2_game PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_ROOT}/output/"
)
