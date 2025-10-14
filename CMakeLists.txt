cmake_minimum_required (VERSION 3.9)

cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0072 NEW)

project (Emulator C CXX)

set(CMAKE_CXX_STANDARD 20)
include(cmake/CompilerWarnings.cmake)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

include(CheckIPOSupported)
check_ipo_supported(RESULT ltoSupport OUTPUT error)

include(FetchContent)
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.92.3-docking
)
FetchContent_Declare(
  imgui_club
  GIT_REPOSITORY https://github.com/ocornut/imgui_club.git
  GIT_TAG        019ed6171a5b46448eb8a2c8e7e8a1d792c722fd
)
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.4
)
FetchContent_Declare(
  rtaudio
  GIT_REPOSITORY https://github.com/thestk/rtaudio.git
  GIT_TAG        6.0.1
)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(imgui imgui_club glfw rtaudio)

find_package(OpenGL REQUIRED)

if(WIN32)
    option(direct "DirectSound" ON)
    option(asio "ASIO" OFF)
    option(wasapi "WASAPI" OFF)

    set(LIBS OpenGL32)
elseif(APPLE)
    option(core "CoreAudio" ON)
    option(jack "Jack Audio Server" OFF)

    set(EXECUTABLE_FLAG MACOSX_BUNDLE)

    set(LIBS pthread "-framework OpenGL") # -framework Cocoa
elseif(UNIX)
    # auto detect installation
    find_package(ALSA QUIET)
    find_package(PulseAudio QUIET)

    option(alsa "ALSA" ${ALSA_FOUND})
    option(pulse "PulseAudio" ${PulseAudio_FOUND})
    option(oss "OSS" OFF)
    option(jack "Jack Audio Server" OFF)

    find_package(GTK2 REQUIRED gtk)

    set(INCLUDES ${GTK2_INCLUDE_DIRS})
    set(LIBS GL ${GTK2_LIBRARIES} stdc++fs pthread)
endif()

if(direct)
    add_definitions(-D__WINDOWS_DS__)
    list(APPEND LIBS dsound)
endif()
if(asio)
    add_definitions(-D__WINDOWS_ASIO__)
endif()
if(wasapi)
    add_definitions(-D__WINDOWS_WASAPI__)
endif()
if(alsa)
    add_definitions(-D__LINUX_ALSA__)
    find_package(ALSA REQUIRED)

    list(APPEND INCLUDES ${ALSA_INCLUDE_DIRS})
    list(APPEND LIBS ${ALSA_LIBRARIES})
endif()
if(pulse)
    add_definitions(-D__LINUX_PULSE__)
    find_package(PulseAudio REQUIRED)

    list(APPEND INCLUDES ${PulseAudio_INCLUDE_DIR})
    list(APPEND LIBS pulse pulse-simple)
endif()
if(oss)
    add_definitions(-D__LINUX_OSS__)
    find_package(OSS REQUIRED)

    list(APPEND INCLUDES ${OSS_INCLUDE_DIRS})
    list(APPEND LIBS ${OSS_LIBRARIES})
endif()
if(jack)
    add_definitions(-D__UNIX_JACK__)
    find_package(Jack REQUIRED)

    list(APPEND INCLUDES ${JACK_INCLUDE_DIRS})
    list(APPEND LIBS ${JACK_LIBRARIES})
endif()
if(core)
    add_definitions(-D__MACOSX_CORE__)

    list(APPEND LIBS "-framework CoreAudio" "-framework CoreFoundation")
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
    "${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp"
    "${rtaudio_SOURCE_DIR}/RtAudio.cpp")

target_include_directories(emu PRIVATE ${imgui_SOURCE_DIR} ${rtaudio_SOURCE_DIR} "${imgui_club_SOURCE_DIR}/imgui_memory_editor" glfw ${INCLUDES})
target_link_libraries(emu PRIVATE glfw ${LIBS})
target_compile_definitions(emu PRIVATE IMGUI_IMPL_OPENGL_LOADER_GL3W)
set_project_warnings(emu)

if(ltoSupport)
    set_property(TARGET emu PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

configure_file("${CMAKE_SOURCE_DIR}/NesCarts (2017-08-21).json" "${CMAKE_CURRENT_BINARY_DIR}/NesCarts (2017-08-21).json" COPYONLY)
