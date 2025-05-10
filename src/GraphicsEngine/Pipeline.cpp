#include "tk/GraphicsEngine/Pipeline.hpp"
#include "tk/ErrorHandling.hpp"

#include <cassert>
#include <fstream>

////////////////////////////////////////////////////////////////////////////////
//                               util
////////////////////////////////////////////////////////////////////////////////

namespace
{

using namespace tk;

auto get_file_data(std::string_view filename)
{
  std::ifstream file(filename.data(), std::ios::ate | std::ios::binary);
  throw_if(!file.is_open(), "failed to open {}", filename);

  auto file_size = (size_t)file.tellg();
  // A SPIR-V module is defined a stream of 32bit words
  auto buffer    = std::vector<uint32_t>(file_size / sizeof(uint32_t));
  
  file.seekg(0);
  file.read((char*)buffer.data(), file_size);

  file.close();
  return buffer;
}

}

namespace tk { namespace graphics_engine {

void Pipeline::destroy() noexcept
{
  assert(_device && _layout && _pipeline);

  vkDestroyPipelineLayout(_device, _layout, nullptr);
  vkDestroyPipeline(_device, _pipeline, nullptr);

  _device   = VK_NULL_HANDLE;
  _layout   = VK_NULL_HANDLE;
  _pipeline = VK_NULL_HANDLE;
}

Pipeline::Pipeline(VkDevice device, std::string_view shader, std::vector<VkDescriptorSetLayout> const& descritptor_layouts, std::vector<VkPushConstantRange> const& push_constants)
{
  assert(device && !_layout && !_pipeline && !shader.empty());

  _device = device;

  VkPipelineLayoutCreateInfo layout_info
  {
    .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount         = static_cast<uint32_t>(descritptor_layouts.size()),
    .pSetLayouts            = descritptor_layouts.data(),
    .pushConstantRangeCount = static_cast<uint32_t>(push_constants.size()),
    .pPushConstantRanges    = push_constants.data(),
  };

  throw_if(vkCreatePipelineLayout(_device, &layout_info, nullptr, &_layout) != VK_SUCCESS,
       "failed to create pipeline layout");

  VkPipelineShaderStageCreateInfo shader_info
  {
    .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage  = VK_SHADER_STAGE_COMPUTE_BIT,
    .module = create_shader_module(shader),
    .pName = "main",
  };

  VkComputePipelineCreateInfo pipeline_info
  {
    .sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
    .stage  = shader_info,
    .layout = _layout,
  };

  // TODO: use pipeline cache can quickly recreate pipeline which have same states
  // TODO: can create multiple pipeline single time
  throw_if(vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &_pipeline) != VK_SUCCESS,
           "failed to create compute pipeline");

  // destroy shader module
  vkDestroyShaderModule(_device, shader_info.module, nullptr);
}

Pipeline::Pipeline(VkDevice device, std::string_view vertex_shader, std::string_view fragment_shader, std::vector<VkDescriptorSetLayout> const& descritptor_layouts, std::vector<VkPushConstantRange> const& push_constants)
{
  // TODO: not move now
  assert(false);
}

auto Pipeline::create_shader_module(std::string_view shader) -> VkShaderModule
{
  assert(_device);

  VkShaderModule module;

  auto data = get_file_data(shader);
  VkShaderModuleCreateInfo info
  {
    .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = data.size() * sizeof(uint32_t),
    .pCode    = reinterpret_cast<uint32_t*>(data.data()),
  };
  throw_if(vkCreateShaderModule(_device, &info, nullptr, &module) != VK_SUCCESS,
           "failed to create shader from {}", shader);
  return module;
}

auto Pipeline::build(VkDevice device, VkPipelineLayout layout) -> VkPipeline
{
  // HACK: can be nullptr for dynamic rendering, see spec
  // HACK: I can't understand why imgui use this structure, can I just use device address?
  VkPipelineVertexInputStateCreateInfo vertex_input_state
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
  };

  // only use triangle list now 
  VkPipelineInputAssemblyStateCreateInfo input_assembly_state
  { 
    .sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
  };

  // HACK: try VK_EXT_extended_dynamic_state3, see spec
  VkPipelineViewportStateCreateInfo viewport_state
  { 
    .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .scissorCount  = 1,
  };

  // HACK: should be discard because of dynamic rendering, see spec
  // set rasterization default option
  _rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
  _rasterization_state.lineWidth   = 1.f; 

  // HACK: can be nullptr for dynamic rendering, see spec
  VkPipelineColorBlendStateCreateInfo color_blend_state
  { 
    .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .attachmentCount = 1,
    .pAttachments    = &_color_blend_attachment,
  };

  // dynamic config
  auto dynamics = std::vector<VkDynamicState>
  {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR,
  };
  VkPipelineDynamicStateCreateInfo dynamic_state
  {
    .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = (uint32_t)dynamics.size(),
    .pDynamicStates    = dynamics.data(),
  };

  // create pipeline
  VkPipeline pipeline;
  VkGraphicsPipelineCreateInfo info
  {
    .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    // use dynamic rendering
    .pNext               = &_rendering_info,
    .stageCount          = (uint32_t)_shader_stages.size(),
    .pStages             = _shader_stages.data(),
    // HACK: can be nullptr for dynamic rendering, see spec
    .pVertexInputState   = &vertex_input_state,
    // HACK: can be nullptr for dynamic rendering, see spec
    .pInputAssemblyState = &input_assembly_state,
    // can be dynamic rendering, but not use now
    .pTessellationState  = nullptr,
    // HACK: can be nullptr for dynamic rendering, see spec
    .pViewportState      = &viewport_state,
    // HACK: can be nullptr for dynamic rendering, see spec
    .pRasterizationState = &_rasterization_state,
    // HACK: can be nullptr for dynamic rendering, see spec
    .pMultisampleState   = &_multisample_state,
    // HACK: can be dynamic rendering
    .pDepthStencilState  = &_depth_stencil_state,
    // HACK: can be nullptr for dynamic rendering, see spec
    .pColorBlendState    = &color_blend_state,
    .pDynamicState       = &dynamic_state,
    .layout              = layout,
  };
  throw_if(vkCreateGraphicsPipelines(device, nullptr, 1, &info, nullptr, &pipeline) != VK_SUCCESS,
           "failed to create pipeline");
  return pipeline;
}

auto Pipeline::clear() -> Pipeline&
{
  _shader_stages.clear();
  _multisample_state      = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO   };
  _rendering_info         = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO           };
  _rasterization_state    = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
  _depth_stencil_state    = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
  _rendering_info_format  = { VK_FORMAT_UNDEFINED };
  _color_blend_attachment =
  {
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                      VK_COLOR_COMPONENT_G_BIT |
                      VK_COLOR_COMPONENT_B_BIT |
                      VK_COLOR_COMPONENT_A_BIT,
  };
  return *this;
}

auto Pipeline::set_msaa(VkSampleCountFlagBits msaa) -> Pipeline&
{
  _multisample_state.rasterizationSamples = msaa;
  _multisample_state.minSampleShading     = 1.f;
  return *this;
}

auto Pipeline::set_color_attachment_format(VkFormat format) -> Pipeline&
{
  _rendering_info_format = format;
  _rendering_info.colorAttachmentCount    = 1;
  _rendering_info.pColorAttachmentFormats = &_rendering_info_format;
  return *this;
}

auto Pipeline::set_shaders(VkShaderModule vertex_shader, VkShaderModule fragment_shader) -> Pipeline&
{
  _shader_stages =
  {
    {
      .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage  = VK_SHADER_STAGE_VERTEX_BIT,
      .module = vertex_shader,
      .pName = "main",
    },
    {
      .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = fragment_shader,
      .pName = "main",
    },
  };
  return *this;
}

auto Pipeline::set_cull_mode(VkCullModeFlags cull_mode, VkFrontFace front_face) -> Pipeline&
{
  _rasterization_state.cullMode  = cull_mode;
  _rasterization_state.frontFace = front_face;
  return *this;
}

auto Pipeline::enable_depth_test(VkFormat format) -> Pipeline&
{
  _rendering_info.depthAttachmentFormat = format;
  _depth_stencil_state.depthTestEnable  = VK_TRUE;
  _depth_stencil_state.depthWriteEnable = VK_TRUE;
  _depth_stencil_state.depthCompareOp   = VK_COMPARE_OP_GREATER_OR_EQUAL;
  _depth_stencil_state.maxDepthBounds   = 1.f;
  return *this;
}

auto Pipeline::enable_additive_blending() -> Pipeline&
{
  _color_blend_attachment.blendEnable = VK_TRUE;
  _color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  _color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
  _color_blend_attachment.colorBlendOp        = VK_BLEND_OP_ADD;
  _color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  _color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  _color_blend_attachment.alphaBlendOp        = VK_BLEND_OP_ADD;
  return *this;
}

auto Pipeline::enable_alpha_blending() -> Pipeline&
{
  _color_blend_attachment.blendEnable = VK_TRUE;
  _color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
  _color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
  _color_blend_attachment.colorBlendOp        = VK_BLEND_OP_ADD;
  _color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  _color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
  _color_blend_attachment.alphaBlendOp        = VK_BLEND_OP_ADD;
  return *this;
}

} }
