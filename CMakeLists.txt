cmake_minimum_required (VERSION 2.6)
project (NES C CXX)

SET(CMAKE_CXX_STANDARD 17)

file(GLOB Common_sources "./src/**/*.cpp") # somehow dosen't match all files
file(GLOB Common_sources1 "./src/*.cpp")

add_executable(nes ${Common_sources} ${Common_sources1}) 
configure_file("./src/shaders/shader.fs" ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file("./src/shaders/shader.vs" ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLM REQUIRED)
find_package(BASS REQUIRED)

include_directories("./src")
target_include_directories(nes 
                            PRIVATE OpenGL
                            PRIVATE GLEW
                            PRIVATE GLFW
                            PRIVATE GLM
                            ${BASS_INCLUDE_DIRS})

target_link_libraries(nes PRIVATE OpenGL GLEW glfw ${BASS_LIBRARIES})
