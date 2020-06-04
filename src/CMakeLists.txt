cmake_minimum_required(VERSION 3.17)
project("Tutorial")
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(DevIL REQUIRED)
find_package(glm REQUIRED)
find_package(fmt REQUIRED)
add_executable("Tutorial" "main.cpp" "glad.c" "Camera.cpp" "CameraManager.cpp" "Lights.cpp" "RandomSampler.cpp")

target_include_directories(${PROJECT_NAME} PRIVATE
                    "${PROJECT_SOURCE_DIR}/../include/"
                     ${OPENGL_INCLUDE_DIRS}
                     ${GLFW3_INCLUDE_DIRS}
                     ${FMT_INCLUDE_DIRS}
                     ${IL_INCLUDE_DIRS}
                     ${GLM_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME}
                      ${OPENGL_LIBRARIES}
                      ${IL_LIBRARIES}
                      glfw
                      dl
                      fmt)


