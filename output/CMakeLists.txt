cmake_minimum_required(VERSION 3.21)
project(PS2RecompiledGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Get the project root directory (parent of output/)
get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)

# Find all generated source files in this directory
file(GLOB GENERATED_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
list(SORT GENERATED_SOURCES)

set(PS2X_OUTPUT_CHUNK_SIZE "256" CACHE STRING "Number of generated output sources to group into one object-library chunk")

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

# Add Windows SDK and VC++ library paths for system libraries (must be before executable definition)
if (MSVC)
    # Get Windows SDK library paths
    file(GLOB SDK_VERSIONS "C:/Program Files (x86)/Windows Kits/10/Lib/*")
    list(SORT SDK_VERSIONS)
    list(REVERSE SDK_VERSIONS)
    list(GET SDK_VERSIONS 0 LATEST_SDK_VERSION)
    string(REPLACE "\\" "/" WINDOWS_SDK_LIB_DIR "${LATEST_SDK_VERSION}")
    
    # Get VC++ library paths - use the latest MSVC version
    set(VC_BASE "C:/Program Files (x86)/Microsoft Visual Studio/18/BuildTools/VC/Tools/MSVC")
    file(GLOB VC_VERSIONS "${VC_BASE}/*")
    list(SORT VC_VERSIONS)
    list(REVERSE VC_VERSIONS)
    list(GET VC_VERSIONS 0 VC_VERSION_PATH)
    string(REPLACE "\\" "/" VC_LIB_DIR "${VC_VERSION_PATH}")
    set(VC_LIB_DIR "${VC_LIB_DIR}/lib/x64")
    
    link_directories("${WINDOWS_SDK_LIB_DIR}/um/x64" "${WINDOWS_SDK_LIB_DIR}/ucrt/x64" "${VC_LIB_DIR}")
endif()

function(ps2_game_apply_common_settings target_name)
    target_include_directories(${target_name} PRIVATE
        "${PROJECT_ROOT}/ps2xRuntime/include"
        "${PROJECT_ROOT}/ps2xRuntime/src/lib"
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PROJECT_ROOT}/out/_deps/fmt-src/include"
        "${PROJECT_ROOT}/out/_deps/raylib-src/src"
    )

    if (MSVC)
        target_compile_options(${target_name} PRIVATE /MP /bigobj)
    endif()
endfunction()

list(LENGTH GENERATED_SOURCES GENERATED_SOURCE_COUNT)
if (GENERATED_SOURCE_COUNT EQUAL 0)
    message(FATAL_ERROR "No generated sources were found in ${CMAKE_CURRENT_SOURCE_DIR}")
endif()

math(EXPR PS2X_OUTPUT_CHUNK_COUNT "(${GENERATED_SOURCE_COUNT} + ${PS2X_OUTPUT_CHUNK_SIZE} - 1) / ${PS2X_OUTPUT_CHUNK_SIZE}")
math(EXPR PS2X_OUTPUT_LAST_CHUNK_INDEX "${PS2X_OUTPUT_CHUNK_COUNT} - 1")
set(PS2_GAME_OBJECT_SOURCES "")

foreach(CHUNK_INDEX RANGE 0 ${PS2X_OUTPUT_LAST_CHUNK_INDEX})
    math(EXPR CHUNK_START "${CHUNK_INDEX} * ${PS2X_OUTPUT_CHUNK_SIZE}")
    math(EXPR CHUNK_REMAINING "${GENERATED_SOURCE_COUNT} - ${CHUNK_START}")
    if (CHUNK_REMAINING LESS_EQUAL 0)
        break()
    endif()

    if (CHUNK_REMAINING GREATER PS2X_OUTPUT_CHUNK_SIZE)
        set(CHUNK_LENGTH ${PS2X_OUTPUT_CHUNK_SIZE})
    else()
        set(CHUNK_LENGTH ${CHUNK_REMAINING})
    endif()

    list(SUBLIST GENERATED_SOURCES ${CHUNK_START} ${CHUNK_LENGTH} CHUNK_SOURCES)
    set(CHUNK_TARGET "ps2_game_chunk_${CHUNK_INDEX}")
    add_library(${CHUNK_TARGET} OBJECT ${CHUNK_SOURCES})
    ps2_game_apply_common_settings(${CHUNK_TARGET})
    list(APPEND PS2_GAME_OBJECT_SOURCES $<TARGET_OBJECTS:${CHUNK_TARGET}>)
endforeach()

# Define the executable as a final link step over chunked object groups.
add_executable(ps2_game)
target_sources(ps2_game PRIVATE ${PS2_GAME_OBJECT_SOURCES})
ps2_game_apply_common_settings(ps2_game)

set_target_properties(ps2_game PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_ROOT}/output"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_ROOT}/output/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_ROOT}/output/Release"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${PROJECT_ROOT}/output/RelWithDebInfo"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${PROJECT_ROOT}/output/MinSizeRel"
)

# Prefer linking directly against sibling targets when output/ is built through the
# top-level project. Standalone output/build still falls back to the known build-tree libs.
if (TARGET ps2_runtime)
    target_link_libraries(ps2_game PRIVATE
        ps2_runtime
        raylib
    )
else()
    # Ninja builds don't use config subdirectories, VS builds do
    if (CMAKE_GENERATOR STREQUAL "Ninja")
        target_link_libraries(ps2_game PRIVATE
            "${PROJECT_ROOT}/out/build/ps2xRuntime/ps2_runtime.lib"
            "${PROJECT_ROOT}/out/build/_deps/raylib-build/raylib/raylib.lib"
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
endif()

target_link_libraries(ps2_game PRIVATE
    winmm.lib
    odbc32.lib
    odbccp32.lib
)
