#pragma once

typedef unsigned char uchar;

// STL
#include <set>
#include <unordered_map>
#include <memory>
#include <vector>
#include <functional>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <cstdlib>
#include <string>
#include <vector>
#include <fstream>
#include <limits>
#include <utility>
#include <optional>
#include <typeindex>

// GLFW
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

// VULKAN
#include <vulkan/vulkan.h>
#include <vks/VulkanInitializers.hpp>
#include <vks/VulkanTools.h>
#include <vks/vk_mem_alloc.hpp>

// IMGUI
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>

// GLM
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_SSE2
#define GLM_FORCE_SWIZZLE
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/perpendicular.hpp>

// SPDLOG
#include <spdlog/spdlog.h>
namespace logger = spdlog;

// TINY OBJ LOADER
#include <tiny_obj_loader.h>

// STB IMAGE
#include <stb_image.h>

// UTILS
namespace lv {
    class NoCopy {
    public:
        NoCopy(NoCopy const&) = delete;
        NoCopy& operator=(NoCopy const&) = delete;
        NoCopy() = default;
    };
}

#define GET(type, var) inline type get_##var() const { return var; }
#define vkCheck(x) VK_CHECK_RESULT(x)
