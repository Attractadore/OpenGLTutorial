find_program(GLSLC NAMES glslc REQUIRED)

set(GLSLC_FLAGS "--target-env=opengl")
separate_arguments(GLSLC_FLAGS NATIVE_COMMAND ${GLSLC_FLAGS})

set(SHADERS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/assets/shaders/src")
set(SHADERS_BINARY_DIR "${CMAKE_SOURCE_DIR}/assets/shaders/bin")

function(add_shader TARGET SHADER)
    set(SHADER_SOURCE_FILE ${SHADERS_SOURCE_DIR}/${SHADER})
    set(SHADER_BINARY_FILE ${SHADERS_BINARY_DIR}/${SHADER}.spv)
    get_filename_component(SHADER_BINARY_DIR ${SHADER_BINARY_FILE} DIRECTORY)
    file(MAKE_DIRECTORY ${SHADER_BINARY_DIR})
    add_custom_command(
        OUTPUT ${SHADER_BINARY_FILE}
        DEPENDS ${SHADER_SOURCE_FILE}
        COMMAND ${GLSLC} ${GLSLC_FLAGS} ${SHADER_SOURCE_FILE} -o ${SHADER_BINARY_FILE}
    )
    target_sources(${TARGET} PRIVATE ${SHADER_BINARY_FILE})
endfunction(add_shader)
