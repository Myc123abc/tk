//
// pipeline builder 
//
// config pipeline then create it 
// use dynamic rendering so don't need framebuffer and render pass
//
// TODO:
//  1. currently only graphics pipeline, after extend to compute pipeline
//  2. try multiple create pipelines at once
//  3. use VK_EXT_extended_dynamic_state3 to make more config become dynamic config
//

#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <string_view>

namespace tk { namespace graphics_engine { 
  
  class Pipeline
  {
  public:
    Pipeline()  = default;
    ~Pipeline() = default;

  private:

    friend class Device;

    // TODO: single create use type decide compute or graphics
    Pipeline(VkDevice device, std::string_view shader, std::vector<VkDescriptorSetLayout> const& descritptor_layouts, std::vector<VkPushConstantRange> const& push_constants);
    Pipeline(VkDevice device, std::string_view vertex_shader, std::string_view fragment_shader, std::vector<VkDescriptorSetLayout> const& descritptor_layouts, std::vector<VkPushConstantRange> const& push_constants);

  private:
    auto create_shader_module(std::string_view shader) -> VkShaderModule;

    // TODO: all device api should in a class Device
    VkDevice         _device   = VK_NULL_HANDLE;
    VkPipelineLayout _layout   = VK_NULL_HANDLE;
    VkPipeline       _pipeline = VK_NULL_HANDLE;

  public:
    void destroy() noexcept;

    operator VkPipeline() const { return _pipeline; }

    // TODO: when use dynamic rendering, can make return type is a class which can use in rendering process
    auto build(VkDevice device, VkPipelineLayout layout)                           -> VkPipeline;
    auto clear()                                                                   -> Pipeline&;

    auto set_msaa(VkSampleCountFlagBits msaa)                                      -> Pipeline&;
    auto set_color_attachment_format(VkFormat format)                              -> Pipeline&;
    // TODO: expand for compute shader
    //       and default use "main" as enter point of shader
    auto set_shaders(VkShaderModule vertex_shader, VkShaderModule fragment_shader) -> Pipeline&;
    // HACK: should be discard because of dynamic rendering, see spec
    auto set_cull_mode(VkCullModeFlags cull_mode, VkFrontFace front_face)          -> Pipeline&;
    auto enable_depth_test(VkFormat format)                                        -> Pipeline&;
    auto enable_additive_blending()                                                -> Pipeline&;
    auto enable_alpha_blending()                                                   -> Pipeline&;

  private:
    std::vector<VkPipelineShaderStageCreateInfo> _shader_stages;
    VkPipelineMultisampleStateCreateInfo         _multisample_state     { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO   };
    VkPipelineRenderingCreateInfo                _rendering_info        { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO           };
    VkPipelineRasterizationStateCreateInfo       _rasterization_state   { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    VkPipelineDepthStencilStateCreateInfo        _depth_stencil_state   { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    VkFormat                                     _rendering_info_format { VK_FORMAT_UNDEFINED };
    VkPipelineColorBlendAttachmentState          _color_blend_attachment
    {
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                        VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT |
                        VK_COLOR_COMPONENT_A_BIT,
    };
 };

} }
