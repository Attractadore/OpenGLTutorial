cmake_minimum_required(VERSION 3.17)
project("OpenGL Tutorial")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
add_subdirectory("src/")

add_custom_target("Shaders" ALL
                   WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/assets/shaders/"
                   COMMAND "${CMAKE_SOURCE_DIR}/assets/shaders/compileShaders.sh")
