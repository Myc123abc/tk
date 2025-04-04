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

namespace tk { namespace graphics_engine { 

  class PipelineBuilder
  {
  public:
    PipelineBuilder()  = default;
    ~PipelineBuilder() = default;

    PipelineBuilder(PipelineBuilder const&)            = delete;
    PipelineBuilder(PipelineBuilder&&)                 = delete;
    PipelineBuilder& operator=(PipelineBuilder const&) = delete;
    PipelineBuilder& operator=(PipelineBuilder&&)      = delete;

    // TODO: when use dynamic rendering, can make return type is a class which can use in rendering process
    auto build(VkDevice device, VkPipelineLayout layout) -> VkPipeline;
    void clear();

    // TODO: expand to multiple attachments
    auto set_color_attachment_format(VkFormat format)                              -> PipelineBuilder&;
    // TODO: expand for compute shader
    //       and default use "main" as enter point of shader
    auto set_shaders(VkShaderModule vertex_shader, VkShaderModule fragment_shader) -> PipelineBuilder&;
    // HACK: should be discard because of dynamic rendering, see spec
    auto set_cull_mode(VkCullModeFlags cull_mode, VkFrontFace front_face)          -> PipelineBuilder&;

  private:
    std::vector<VkPipelineShaderStageCreateInfo> _shader_stages;
    VkPipelineRenderingCreateInfo                _rendering_info        { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO            };
    VkPipelineRasterizationStateCreateInfo       _rasterization_state   { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO  };
    VkFormat                                     _rendering_info_format { VK_FORMAT_UNDEFINED };
  };

} }
