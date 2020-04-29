cmake_minimum_required (VERSION 2.6)
project (NES C CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)

add_subdirectory("./extern/glm")

set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")
add_subdirectory("./extern/json")

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
    set(LIBS OpenGL ${GTK2_LIBRARIES} stdc++fs pthread glfw)
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

file(GLOB_RECURSE Common_sources "./src/**/*.cpp" "./src/*.cpp")
file(GLOB imgui_sources "./extern/imgui/*.cpp")
add_executable(nes WIN32 ${Common_sources} ${imgui_sources} 
                         "./extern/imgui/examples/imgui_impl_opengl3.cpp" 
                         "./extern/imgui/examples/imgui_impl_glfw.cpp"
                         "./extern/rtaudio/RtAudio.cpp")

target_include_directories(nes PRIVATE "src" "extern/imgui" "extern/rtaudio" ${GLEW_INCLUDE_DIRS} glfw ${INCLUDES})
target_link_libraries(nes PRIVATE ${GLEW_LIBRARIES} glfw glm::glm nlohmann_json::nlohmann_json ${LIBS})
