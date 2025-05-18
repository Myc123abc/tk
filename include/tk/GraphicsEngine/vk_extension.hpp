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
inline PFN_vkCreateShadersEXT                       vkCreateShadersEXT{};
inline PFN_vkDestroyShaderEXT                       vkDestroyShaderEXT{};
inline PFN_vkCmdBindShadersEXT                      vkCmdBindShadersEXT{};
inline PFN_vkCmdSetCullModeEXT                      vkCmdSetCullModeEXT{};
inline PFN_vkCmdSetDepthWriteEnableEXT              vkCmdSetDepthWriteEnableEXT{};
inline PFN_vkCmdSetPolygonModeEXT                   vkCmdSetPolygonModeEXT{};
inline PFN_vkCmdSetRasterizationSamplesEXT          vkCmdSetRasterizationSamplesEXT{};
inline PFN_vkCmdSetSampleMaskEXT                    vkCmdSetSampleMaskEXT{};
inline PFN_vkCmdSetAlphaToCoverageEnableEXT         vkCmdSetAlphaToCoverageEnableEXT{};
inline PFN_vkCmdSetVertexInputEXT                   vkCmdSetVertexInputEXT{};
inline PFN_vkCmdSetColorBlendEnableEXT              vkCmdSetColorBlendEnableEXT{};
inline PFN_vkCmdSetColorWriteMaskEXT                vkCmdSetColorWriteMaskEXT{};

}}