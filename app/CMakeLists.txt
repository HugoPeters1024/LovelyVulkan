cmake_minimum_required(VERSION 3.19)
project(app)

macro(shader)
    add_custom_command(
            OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/shaders_bin/${ARGV0}.spv
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/${ARGV0}
            COMMAND /usr/bin/glslc
            ARGS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/${ARGV0} -o ${CMAKE_CURRENT_SOURCE_DIR}/shaders_bin/${ARGV0}.spv -O
            COMMENT building shaders
            VERBATIM)
    SET(src ${src} ${CMAKE_CURRENT_SOURCE_DIR}/shaders_bin/${ARGV0}.spv)
endmacro()

shader("test.comp")

set(CMAKE_CXX_STANDARD 20)
add_executable(app main.cpp)