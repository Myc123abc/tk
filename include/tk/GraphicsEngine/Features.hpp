#pragma once

#include <vulkan/vulkan.h>
#include "config.hpp"

namespace tk { namespace graphics_engine {

class Features
{
public:
  Features()
  {
    auto cfg = config();

    _features2.pNext = &_features12;

    _features12.pNext                                     = &_features13;
    _features12.bufferDeviceAddress                       = VK_TRUE;
    _features12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    _features12.runtimeDescriptorArray                    = VK_TRUE;

    _features13.synchronization2 = VK_TRUE;
    _features13.dynamicRendering = VK_TRUE;
  }

  auto get() const noexcept
  {
    return &_features2;
  }

private:
  VkPhysicalDeviceVulkan13Features _features13{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
  VkPhysicalDeviceVulkan12Features _features12{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
  VkPhysicalDeviceFeatures2        _features2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
};

}}