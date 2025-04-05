#pragma once

#include "Buffer.hpp"

#include <vulkan/vulkan.h>

#include <vector>

namespace tk { namespace graphics_engine {

inline constexpr auto Vulkan_Version = VK_API_VERSION_1_4;

inline const std::vector<const char*> Device_Extensions = 
{
  VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

inline constexpr uint32_t Max_Frame_Number = 2;

inline std::vector<Vertex> Vertices
{
  { {  .5f, -.5f,  0.f }, {}, { 0.f, 0.f, 0.f, 1.f } },
  { {  .5f,  .5f,  0.f }, {}, { .5f, .5f, .5f, 1.f } },
  { { -.5f, -.5f,  0.f }, {}, { 1.f, 0.f, 0.f, 1.f } },
  { { -.5f,  .5f,  0.f }, {}, { 0.f, 1.f, 0.f, 1.f } },
};

inline std::vector<uint32_t> Indices 
{
  0, 1, 2,
  2, 1, 3,
};

} }
