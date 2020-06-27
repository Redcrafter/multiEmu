cmake_minimum_required (VERSION 2.6)
project (NES C CXX)

set(CMAKE_CXX_STANDARD 17)

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
    option(alsa "ALSA" ON)
    option(pulse "PulseAudio" OFF)
    option(oss "OSS" OFF)
    option(jack "Jack Audio Server" OFF)

    find_package(GTK2 REQUIRED gtk)

    set(INCLUDES ${GTK2_INCLUDE_DIRS})
    set(LIBS GL ${GTK2_LIBRARIES} stdc++fs pthread)

    set(CMAKE_CXX_FLAGS_DEBUG "-Wall -Wextra -Wno-unknown-pragmas -Wno-switch -Wno-unused-parameter -Wno-implicit-fallthrough -g")
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
    
    list(APPEND INCLUDES ${ALSA_INCLUDE_DIR})
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

#if(!(alsa or pulse or oss or jack or core))
#    message(WARNING "No Audio Library selected")
#endif()
add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GL3W)

file(GLOB_RECURSE Common_sources "./src/**/*.cpp" "./src/*.cpp")
file(GLOB imgui_sources "./extern/imgui/*.cpp")
add_executable(nes WIN32 ${Common_sources} ${imgui_sources} 
                         "./extern/imgui/examples/imgui_impl_opengl3.cpp" 
                         "./extern/imgui/examples/imgui_impl_glfw.cpp"
                         "./extern/imgui/examples/libs/gl3w/GL/gl3w.c"
                         "./extern/rtaudio/RtAudio.cpp")

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/glfw)

target_include_directories(nes PRIVATE "src" "extern/imgui" "extern/imgui/examples/libs/gl3w" "extern/rtaudio"  glfw ${INCLUDES})
target_link_libraries(nes PRIVATE glfw ${LIBS})

configure_file("${CMAKE_SOURCE_DIR}/NesCarts (2017-08-21).json" "${CMAKE_CURRENT_BINARY_DIR}/NesCarts (2017-08-21).json" COPYONLY)
