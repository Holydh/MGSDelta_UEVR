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
file(GLOB_RECURSE PLUGIN_SOURCES CONFIGURE_DEPENDS
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

add_library(mgsdelta_uevr SHARED
    ${PLUGIN_SOURCES}
    ${EXTERNAL_SOURCES}
    ${EXTERNAL_HEADERS})

target_include_directories(mgsdelta_uevr PRIVATE
    src
    external)

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

# Each configuration links into its own folder, then the DLL + PDB of the
# configuration just built is copied into the UEVR profile's plugins folder.
set_target_properties(mgsdelta_uevr PROPERTIES
    OUTPUT_NAME "MGSDelta_Plugin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")

add_custom_command(TARGET mgsdelta_uevr POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${MGSDELTA_PLUGIN_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:mgsdelta_uevr>" "$<TARGET_PDB_FILE:mgsdelta_uevr>" "${MGSDELTA_PLUGIN_DIR}"
    COMMENT "Deploying $<CONFIG> MGSDelta_Plugin.dll to ${MGSDELTA_PLUGIN_DIR}")

# Never-produced output: keeps the project from being skipped as "up to date",
# so switching Debug <-> Release and building always redeploys the right DLL.
set(DEPLOY_TRIGGER "${CMAKE_BINARY_DIR}/always_redeploy")
add_custom_command(OUTPUT "${DEPLOY_TRIGGER}" COMMAND ${CMAKE_COMMAND} -E echo_append VERBATIM)
set_source_files_properties("${DEPLOY_TRIGGER}" PROPERTIES SYMBOLIC TRUE)
target_sources(mgsdelta_uevr PRIVATE "${DEPLOY_TRIGGER}")

# Keep a PDB for Release too, so crashes in-game can be symbolized
target_compile_options(mgsdelta_uevr PRIVATE $<$<CONFIG:Release>:/Zi>)
target_link_options(mgsdelta_uevr PRIVATE $<$<CONFIG:Release>:/DEBUG /OPT:REF /OPT:ICF>)

# IDE niceties
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "src" FILES ${PLUGIN_SOURCES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/external" PREFIX "external"
    FILES ${EXTERNAL_SOURCES} ${EXTERNAL_HEADERS})
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT mgsdelta_uevr)
