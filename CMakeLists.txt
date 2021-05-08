cmake_minimum_required(VERSION 3.18)
project("OpenGL Tutorial")

include("cmake/CMakeTemplates/CFlags.cmake")
include("cmake/CMakeTemplates/CXXFlags.cmake")
include("cmake/shader.cmake")
include("modules/glad2/cmake/CMakeLists.txt")

find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(assimp REQUIRED)
find_package(PNG REQUIRED)

glad_add_library(GL46 SHARED LANGUAGE C API gl:core=4.6 EXTENSIONS NONE)

add_library(Camera SHARED "src/Camera.cpp")
target_include_directories(Camera PUBLIC "include")
target_link_libraries(Camera PUBLIC glm::glm)

add_library(CameraManager SHARED "src/CameraManager.cpp")
target_include_directories(CameraManager PUBLIC "include")
target_link_libraries(CameraManager PUBLIC glfw Camera)

add_library(LoadModel SHARED "src/load_model.cpp")
target_include_directories(LoadModel PUBLIC "include")
target_link_libraries(LoadModel PUBLIC GL46 glm::glm PRIVATE assimp::assimp)

add_library(LoadShader SHARED "src/load_shader.cpp")
target_include_directories(LoadShader PUBLIC "include")
target_link_libraries(LoadShader PUBLIC GL46)

add_library(LoadTexture SHARED "src/load_texture.cpp")
target_include_directories(LoadTexture PUBLIC "include")
target_link_libraries(LoadTexture PUBLIC GL46 PRIVATE PNG::PNG)

add_executable(blending "src/blending.cpp")
target_link_libraries(blending PRIVATE CameraManager LoadModel LoadShader LoadTexture)
add_shader(blending "blending/diffuse.vert")
add_shader(blending "blending/diffuse.frag")

add_executable(cubemaps "src/cubemaps.cpp")
target_link_libraries(cubemaps PRIVATE CameraManager LoadModel LoadShader LoadTexture)
add_shader(cubemaps "cubemaps/skybox.vert")
add_shader(cubemaps "cubemaps/skybox.frag")
add_shader(cubemaps "cubemaps/mirror.vert")
add_shader(cubemaps "cubemaps/mirror.frag")

add_executable(shadows "src/shadows.cpp" "assets/shaders/src/shadows/indices.glsl")
target_include_directories(shadows PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(shadows PRIVATE CameraManager LoadModel LoadShader)
add_shader(shadows "shadows/lighting.vert")
add_shader(shadows "shadows/lighting.frag")
add_shader(shadows "shadows/shadow.vert")
add_shader(shadows "shadows/shadow.geom")
add_shader(shadows "shadows/ZPrepass.frag")
add_shader(shadows "shadows/ZPartition.comp")

add_executable(instancing "src/instancing.cpp")
target_link_libraries(instancing PRIVATE CameraManager LoadModel LoadShader)
add_shader(shadows "instancing/diffuse.vert")
add_shader(shadows "instancing/diffuse_instanced.vert")
add_shader(shadows "instancing/diffuse.frag")
