project(vks CXX)
add_library(vks
        ${CMAKE_CURRENT_SOURCE_DIR}/vks/VulkanInitializers.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/vks/vk_mem_alloc.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/vks/VulkanTools.h
        ${CMAKE_CURRENT_SOURCE_DIR}/vks/VulkanTools.cpp
        )
set_target_properties(vks PROPERTIES LINKER_LANGUAGE CXX)
