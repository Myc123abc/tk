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

    _features12.pNext                                        = &_features13;
    _features12.bufferDeviceAddress                          = VK_TRUE;
    _features12.shaderSampledImageArrayNonUniformIndexing    = VK_TRUE;
    _features12.runtimeDescriptorArray                       = VK_TRUE;
    _features12.descriptorBindingVariableDescriptorCount     = VK_TRUE;

    _features13.synchronization2 = VK_TRUE;
    _features13.dynamicRendering = VK_TRUE;

    auto p_next = &_features13.pNext;

    if (cfg->use_descriptor_buffer)
    {
      *p_next                             = &_descriptor_buffer;
      p_next                              = &_descriptor_buffer.pNext;
      _descriptor_buffer.descriptorBuffer = VK_TRUE;
    }

    if (cfg->use_shader_object)
    {
      *p_next                     = &_shader_object;
      p_next                      = &_shader_object.pNext;
      _shader_object.shaderObject = VK_TRUE;
    }
  }

  auto get() const noexcept
  {
    return &_features2;
  }

private:
  VkPhysicalDeviceShaderObjectFeaturesEXT     _shader_object{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT };
  VkPhysicalDeviceDescriptorBufferFeaturesEXT _descriptor_buffer{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT };
  VkPhysicalDeviceVulkan13Features            _features13{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
  VkPhysicalDeviceVulkan12Features            _features12{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
  VkPhysicalDeviceFeatures2                   _features2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
};

}}