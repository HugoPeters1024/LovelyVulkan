cmake_minimum_required(VERSION 3.19)
project(lovelyvulkan)

set(CMAKE_CXX_STANDARD 20)
file(GLOB_RECURSE src CONFIGURE_DEPENDS "include/*.h" "include/*.hpp" "src/*.cpp")


add_library(lovelyvulkan ${src})
target_precompile_headers(lovelyvulkan PRIVATE include/precomp.h)
target_include_directories(lovelyvulkan PRIVATE include)

add_subdirectory(${CMAKE_SOURCE_DIR}/external/vks)
link_directories(${CMAKE_SOURCE_DIR}/external/vks)
include_directories(lovelyvulkan PUBLIC ${CMAKE_SOURCE_DIR}/external/vks/)
target_link_libraries(lovelyvulkan vks)

add_subdirectory(${CMAKE_SOURCE_DIR}/external/glfw)
link_directories(${CMAKE_SOURCE_DIR}/external/glfw)
include_directories(lovelyvulkan PUBLIC ${CMAKE_SOURCE_DIR}/external/glfw/include/)
target_link_libraries(lovelyvulkan glfw)

add_subdirectory(${CMAKE_SOURCE_DIR}/external/glm)
link_directories(${CMAKE_SOURCE_DIR}/external/glm)
include_directories(lovelyvulkan PUBLIC ${CMAKE_SOURCE_DIR}/external/glm/glm/)
target_link_libraries(lovelyvulkan glm)


add_subdirectory(${CMAKE_SOURCE_DIR}/external/imgui)
link_directories(${CMAKE_SOURCE_DIR}/external/imgui)
include_directories(lovelyvulkan PRIVATE ${CMAKE_SOURCE_DIR}/external/imgui/)
target_link_libraries(lovelyvulkan imgui)

add_subdirectory(${CMAKE_SOURCE_DIR}/external/spdlog)
link_directories(${CMAKE_SOURCE_DIR}/external/spdlog)
include_directories(lovelyvulkan PRIVATE ${CMAKE_SOURCE_DIR}/external/spdlog/include/)
target_link_libraries(lovelyvulkan spdlog)

include_directories(lovelyvulkan PRIVATE ${CMAKE_SOURCE_DIR}/external/tiny_obj_loader)
include_directories(lovelyvulkan PRIVATE ${CMAKE_SOURCE_DIR}/external/stb_image)

find_package(Vulkan REQUIRED)
target_link_libraries(lovelyvulkan Vulkan::Vulkan)

add_subdirectory(app)
target_include_directories(app PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(app lovelyvulkan)

