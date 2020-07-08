cmake_minimum_required(VERSION 3.17)
project("Tutorial")
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(assimp REQUIRED)
find_package(Boost REQUIRED)
find_package(PNG REQUIRED)
add_executable("Tutorial" "main.cpp" "glad.c" "Camera.cpp" "CameraManager.cpp" "Lights.cpp" "RandomSampler.cpp" "TextureLoader.cpp")

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
                    "${PROJECT_SOURCE_DIR}/../include/"
                     ${OPENGL_INCLUDE_DIRS}
                     ${GLFW3_INCLUDE_DIRS}
                     ${FMT_INCLUDE_DIRS}
                     ${GLM_INCLUDE_DIRS}
                     ${ASSIMP_INCLUDE_DIRS}
                     ${BOOST_INCLUDE_DIRS}
                     ${PNG_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME}
                      ${OPENGL_LIBRARIES}
                      png
                      assimp
                      glfw)
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    target_link_libraries(${PROJECT_NAME} dl)
endif()
