cmake_minimum_required(VERSION 3.24)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

project(mgsdelta_uevr LANGUAGES C CXX)

if(NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "The MGSDelta UEVR plugin must be built with MSVC for x64.")
endif()

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
set(MGSDELTA_PLUGIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/UEVR_profile/MGSDelta-Win64-Shipping/plugins"
    CACHE PATH "Folder MGSDelta_Plugin.dll is written to (the UEVR profile's plugins folder)")

# TEMPORARY: local Dumper-7 SDK dump. Not in git; being replaced by UEVR API calls.
set(MGSDELTA_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/sdk"
    CACHE PATH "Folder containing the Dumper-7 SDK dump (SDK.hpp, SDK/...)")

# ---------------------------------------------------------------------------
# Global settings (also applied to fetched dependencies)
# ---------------------------------------------------------------------------
# Static C/C++ runtime (/MT, /MTd): the plugin DLL does not depend on whatever
# msvcp140.dll / vcruntime140.dll version the game process has loaded.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)

add_compile_options(/MP)

# ---------------------------------------------------------------------------
# Dependencies (downloaded at configure time)
# ---------------------------------------------------------------------------
include(FetchContent)

# bddisasm and spdlog are required by kananlib
FetchContent_Declare(bddisasm
    GIT_REPOSITORY https://github.com/bitdefender/bddisasm
    GIT_TAG        v1.34.10
    GIT_SHALLOW    TRUE)

FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog
    GIT_TAG        76fb40d95455f249bd70824ecfcae7a8f0930fa3)

FetchContent_Declare(kananlib
    GIT_REPOSITORY https://github.com/cursey/kananlib
    GIT_TAG        153b177593a97042a57608b36d4253fa5e982914)

FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm
    GIT_TAG        1.0.3
    GIT_SHALLOW    TRUE)

FetchContent_MakeAvailable(bddisasm spdlog kananlib glm)

# ---------------------------------------------------------------------------
# Plugin
# ---------------------------------------------------------------------------
file(GLOB PLUGIN_SOURCES CONFIGURE_DEPENDS
    src/*.cpp
    src/*.hpp)

set(RENDERLIB_DIR external/renderlib)
set(EXTERNAL_SOURCES
    # Dear ImGui core
    ${RENDERLIB_DIR}/IMGUI/imgui.cpp
    ${RENDERLIB_DIR}/IMGUI/imgui_demo.cpp
    ${RENDERLIB_DIR}/IMGUI/imgui_draw.cpp
    ${RENDERLIB_DIR}/IMGUI/imgui_tables.cpp
    ${RENDERLIB_DIR}/IMGUI/imgui_widgets.cpp
    # ImGui backends adapted for UEVR
    ${RENDERLIB_DIR}/UEVR_IMGUI/imgui_impl_dx11.cpp
    ${RENDERLIB_DIR}/UEVR_IMGUI/imgui_impl_dx12.cpp
    ${RENDERLIB_DIR}/UEVR_IMGUI/imgui_impl_win32.cpp
    # D3D11/D3D12 render hooks
    ${RENDERLIB_DIR}/RENDERING/d3d11.cpp
    ${RENDERLIB_DIR}/RENDERING/d3d12.cpp)

file(GLOB_RECURSE EXTERNAL_HEADERS CONFIGURE_DEPENDS
    external/*.h
    external/*.hpp)

# TEMPORARY: Dumper-7 SDK
if(NOT EXISTS "${MGSDELTA_SDK_DIR}/SDK.hpp")
    message(FATAL_ERROR
        "Dumper-7 SDK dump not found in '${MGSDELTA_SDK_DIR}'.\n"
        "It is still required until the SDK usages are ported to the UEVR API. "
        "Set MGSDELTA_SDK_DIR to the dump folder (the one containing SDK.hpp).")
endif()
set(SDK_SOURCES
    ${MGSDELTA_SDK_DIR}/SDK/Basic.cpp
    ${MGSDELTA_SDK_DIR}/SDK/BP_SnakeStatus_functions.cpp
    ${MGSDELTA_SDK_DIR}/SDK/CobraUI_functions.cpp
    ${MGSDELTA_SDK_DIR}/SDK/Cobra_functions.cpp
    ${MGSDELTA_SDK_DIR}/SDK/CoreUObject_functions.cpp
    ${MGSDELTA_SDK_DIR}/SDK/Engine_functions.cpp
    ${MGSDELTA_SDK_DIR}/SDK/Gsr_functions.cpp
    ${MGSDELTA_SDK_DIR}/SDK/SightDragnovWidget_functions.cpp
    ${MGSDELTA_SDK_DIR}/SDK/SightMosinWidget_functions.cpp
    ${MGSDELTA_SDK_DIR}/SDK/UMG_functions.cpp)

add_library(mgsdelta_uevr SHARED
    ${PLUGIN_SOURCES}
    ${EXTERNAL_SOURCES}
    ${EXTERNAL_HEADERS}
    ${SDK_SOURCES})

target_include_directories(mgsdelta_uevr PRIVATE
    src
    external
    ${MGSDELTA_SDK_DIR})

target_link_libraries(mgsdelta_uevr PRIVATE
    kananlib
    glm::glm)

target_compile_features(mgsdelta_uevr PRIVATE cxx_std_23)

target_compile_options(mgsdelta_uevr PRIVATE
    /bigobj
    /EHa
    /GS-)

target_compile_definitions(mgsdelta_uevr PRIVATE
    NOMINMAX
    WINVER=0x0A00
    GLM_ENABLE_EXPERIMENTAL)  # code uses glm/gtx/*

# Release is written straight into the UEVR profile's plugins folder.
# Debug stays in the build folder: UEVR loads every DLL in plugins/, and sharing
# one output path would let a Debug build silently replace the Release one.
set_target_properties(mgsdelta_uevr PROPERTIES
    OUTPUT_NAME "MGSDelta_Plugin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${MGSDELTA_PLUGIN_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug")

# Keep a PDB for Release too, so crashes in-game can be symbolized
target_compile_options(mgsdelta_uevr PRIVATE $<$<CONFIG:Release>:/Zi>)
target_link_options(mgsdelta_uevr PRIVATE $<$<CONFIG:Release>:/DEBUG /OPT:REF /OPT:ICF>)

# IDE niceties
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "src" FILES ${PLUGIN_SOURCES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/external" PREFIX "external"
    FILES ${EXTERNAL_SOURCES} ${EXTERNAL_HEADERS})
source_group("sdk (temporary)" FILES ${SDK_SOURCES})
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT mgsdelta_uevr)
