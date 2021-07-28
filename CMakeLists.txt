cmake_minimum_required(VERSION 3.19)
project(liftedvulkan)

set(CMAKE_CXX_STANDARD 20)
file(GLOB_RECURSE src CONFIGURE_DEPENDS "include/*.h" "include/*.hpp" "src/*.cpp")

macro(shader)
    add_custom_command(
            OUTPUT assets/shaders_bin/${ARGV0}.spv
            DEPENDS assets/shaders/${ARGV0}
            COMMAND /usr/bin/glslc
            ARGS ${CMAKE_SOURCE_DIR}/assets/shaders/${ARGV0} -o ${CMAKE_SOURCE_DIR}/assets/shaders_bin/${ARGV0}.spv -O
            COMMENT building shaders
            VERBATIM)
    SET(src ${src} assets/shaders_bin/${ARGV0}.spv)
endmacro()

add_library(liftedvulkan ${src})
target_precompile_headers(liftedvulkan PRIVATE include/precomp.h)
target_include_directories(liftedvulkan PRIVATE include)

add_subdirectory(${CMAKE_SOURCE_DIR}/external/vks)
link_directories(${CMAKE_SOURCE_DIR}/external/vks)
include_directories(liftedvulkan PUBLIC ${CMAKE_SOURCE_DIR}/external/vks/)
target_link_libraries(liftedvulkan vks)

add_subdirectory(${CMAKE_SOURCE_DIR}/external/glfw)
link_directories(${CMAKE_SOURCE_DIR}/external/glfw)
include_directories(liftedvulkan PUBLIC ${CMAKE_SOURCE_DIR}/external/glfw/include/)
target_link_libraries(liftedvulkan glfw)

add_subdirectory(${CMAKE_SOURCE_DIR}/external/glm)
link_directories(${CMAKE_SOURCE_DIR}/external/glm)
include_directories(liftedvulkan PUBLIC ${CMAKE_SOURCE_DIR}/external/glm/glm/)
target_link_libraries(liftedvulkan glm)


add_subdirectory(${CMAKE_SOURCE_DIR}/external/imgui)
link_directories(${CMAKE_SOURCE_DIR}/external/imgui)
include_directories(liftedvulkan PRIVATE ${CMAKE_SOURCE_DIR}/external/imgui/)
target_link_libraries(liftedvulkan imgui)

add_subdirectory(${CMAKE_SOURCE_DIR}/external/spdlog)
link_directories(${CMAKE_SOURCE_DIR}/external/spdlog)
include_directories(liftedvulkan PRIVATE ${CMAKE_SOURCE_DIR}/external/spdlog/include/)
target_link_libraries(liftedvulkan spdlog)

find_package(Vulkan REQUIRED)
target_link_libraries(liftedvulkan Vulkan::Vulkan)

add_subdirectory(app)
target_include_directories(app PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(app liftedvulkan)