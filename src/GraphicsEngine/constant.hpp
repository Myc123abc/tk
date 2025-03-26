#pragma once

#include "ShaderStructs.hpp"

#include <vulkan/vulkan.h>

#include <vector>

namespace tk
{

inline constexpr auto Vulkan_Version = VK_API_VERSION_1_4;

inline const std::vector<const char*> Device_Extensions = 
{
  VK_KHR_SWAPCHAIN_EXTENSION_NAME,
  // swapchain maintenance extension can auto recreate swapchain
  VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME,
};

inline constexpr uint32_t Max_Frame_Number = 2;

inline const std::vector<Vertex> Vertices
{
  // { { -1.f, -1.f }, { 1.f, 0.f, 0.f }, },
  // { {  1.f, -1.f }, { 0.f, 1.f, 0.f }, },
  // { {  1.f,  1.f }, { 0.f, 0.f, 1.f }, },
  // { { -1.f,  1.f }, { 1.f, 1.f, 1.f }, },
  { { -.5f, -.5f,  0.f }, { 1.f, 0.f, 0.f }, },
  { {  .5f, -.5f,  0.f }, { 0.f, 1.f, 0.f }, },
  { {  .5f,  .5f,  5.f }, { 0.f, 0.f, 1.f }, },
  { { -.5f,  .5f,  0.f }, { 1.f, 1.f, 1.f }, },
  // { {  .0f, -1.f,  0.f }, { 1.f, 0.f, 0.f }, },
  // { { -1.f,  1.f,  0.f }, { 0.f, 1.f, 0.f }, },
  // { {  1.f,  1.f,  5.f }, { 0.f, 0.f, 0.f }, },
};

inline const std::vector<uint16_t> Indices 
{
  0, 1, 2,
  0, 2, 3,
};

}
