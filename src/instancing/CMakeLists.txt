cmake_minimum_required(VERSION 3.17)
project("instancing")
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(assimp REQUIRED)
add_executable(${PROJECT_NAME}
                "main.cpp"
                "../common/glad.c"
                "../common/Camera.cpp"
                "../common/CameraManager.cpp"
                "../common/load_model.cpp"
                "../common/load_shader.cpp")

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
                    "${PROJECT_SOURCE_DIR}/../../include/"
                     ${OPENGL_INCLUDE_DIRS}
                     ${GLFW3_INCLUDE_DIRS}
                     ${GLM_INCLUDE_DIRS}
                     ${ASSIMP_INCLUDE_DIRS}
                     ${PNG_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME}
                      ${OPENGL_LIBRARIES}
                      glfw
                      assimp)
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    target_link_libraries(${PROJECT_NAME} dl)
endif()
