#pragma once

#include <vulkan/vulkan.h>

namespace tk { namespace graphics_engine {

inline PFN_vkCreateDebugUtilsMessengerEXT           vkCreateDebugUtilsMessengerEXT{};
inline PFN_vkDestroyDebugUtilsMessengerEXT          vkDestroyDebugUtilsMessengerEXT{};
inline PFN_vkGetDescriptorSetLayoutSizeEXT          vkGetDescriptorSetLayoutSizeEXT{};
inline PFN_vkGetDescriptorSetLayoutBindingOffsetEXT vkGetDescriptorSetLayoutBindingOffsetEXT{};
inline PFN_vkGetDescriptorEXT                       vkGetDescriptorEXT{};
inline PFN_vkCmdBindDescriptorBuffersEXT            vkCmdBindDescriptorBuffersEXT{};
inline PFN_vkCmdSetDescriptorBufferOffsetsEXT       vkCmdSetDescriptorBufferOffsetsEXT{};

}}