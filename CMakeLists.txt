cmake_minimum_required (VERSION 3.30)

project (Emulator C CXX)

include(cmake/CompilerWarnings.cmake)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

include(CheckIPOSupported)
check_ipo_supported(RESULT ltoSupport OUTPUT error)

SET(SDL_TEST_LIBRARY OFF)
SET(SDL_SHARED OFF)
SET(SDL_STATIC ON)

SET(SDL_X11_XSCRNSAVER OFF)
SET(SDL_X11_XTEST OFF)

include(FetchContent)
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.92.8-docking
)
FetchContent_Declare(
  imgui_club
  GIT_REPOSITORY https://github.com/ocornut/imgui_club.git
  GIT_TAG        a7eab6ccb9fec09f37705406a06bb3bfc09597fe
)
FetchContent_Declare(
  sdl3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG        release-3.4.12
)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(imgui imgui_club json sdl3)

if(WIN32)
    set(LIBS OpenGL32)
elseif(APPLE)
    set(EXECUTABLE_FLAG MACOSX_BUNDLE)

    set(LIBS pthread "-framework OpenGL") # -framework Cocoa
elseif(UNIX)
    set(LIBS GL stdc++fs pthread)
endif()

file(GLOB_RECURSE Common_sources
    "./src/*.cpp" "./src/**/*.cpp"
    "./src/*.hpp" "./src/**/*.hpp"
    "./src/*.ixx" "./src/**/*.ixx"
    "./src/*.h"   "./src/**/*.h")

file(GLOB imgui_sources "${imgui_SOURCE_DIR}/*.cpp")

source_group(TREE "${CMAKE_SOURCE_DIR}/src" FILES ${Common_sources})

add_executable(emu WIN32 ${Common_sources} ${imgui_sources}
    "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp")

target_include_directories(emu PRIVATE ${imgui_SOURCE_DIR} "${imgui_club_SOURCE_DIR}/imgui_memory_editor" ${INCLUDES})
target_link_libraries(emu PRIVATE ${LIBS} nlohmann_json::nlohmann_json SDL3::SDL3-static)
target_compile_definitions(emu PRIVATE IMGUI_IMPL_OPENGL_LOADER_GL3W)
set_project_warnings(emu)
target_compile_features(emu PRIVATE cxx_std_20)

if(ltoSupport)
    set_property(TARGET emu PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

configure_file("${CMAKE_SOURCE_DIR}/NesCarts (2017-08-21).json" "${CMAKE_CURRENT_BINARY_DIR}/NesCarts (2017-08-21).json" COPYONLY)
