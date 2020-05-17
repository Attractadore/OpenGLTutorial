cmake_minimum_required(VERSION 3.17)
project("Tutorial")
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
add_executable("Tutorial" "main.cpp" "glad.c")

target_include_directories(${PROJECT_NAME} PRIVATE
                    "${PROJECT_SOURCE_DIR}/../include/"
                     ${OPENGL_INCLUDE_DIRS}
                     ${GLFW3_INCLUDE_DIRS})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
target_link_libraries(${PROJECT_NAME}
                      ${OPENGL_LIBRARIES}
                      glfw
                      dl)


