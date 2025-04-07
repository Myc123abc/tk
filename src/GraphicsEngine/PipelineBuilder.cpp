#include "PipelineBuilder.hpp"
#include "ErrorHandling.hpp"

namespace tk { namespace graphics_engine {

auto PipelineBuilder::build(VkDevice device, VkPipelineLayout layout) -> VkPipeline
{
  // HACK: can be nullptr for dynamic rendering, see spec
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

  // TODO: use it in feature and it can be dynamic rendering, default multisample option
  VkPipelineMultisampleStateCreateInfo multisample_state
  { 
    .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    .minSampleShading     = 1.f,
  };

  // HACK: can be nullptr for dynamic rendering, see spec
  VkPipelineColorBlendAttachmentState attachment 
  {
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                      VK_COLOR_COMPONENT_G_BIT |
                      VK_COLOR_COMPONENT_B_BIT |
                      VK_COLOR_COMPONENT_A_BIT,
  };
  VkPipelineColorBlendStateCreateInfo color_blend_state
  { 
    .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .logicOp         = VK_LOGIC_OP_COPY,
    .attachmentCount = 1,
    .pAttachments    = &attachment,
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
    .pMultisampleState   = &multisample_state,
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

auto PipelineBuilder::clear() -> PipelineBuilder&
{
  _shader_stages.clear();
  _rendering_info        = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO           };
  _rasterization_state   = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
  _depth_stencil_state   = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
  _rendering_info_format = { VK_FORMAT_UNDEFINED };
  return *this;
}

auto PipelineBuilder::set_color_attachment_format(VkFormat format) -> PipelineBuilder&
{
  _rendering_info_format = format;
  _rendering_info.colorAttachmentCount    = 1;
  _rendering_info.pColorAttachmentFormats = &_rendering_info_format;
  return *this;
}

auto PipelineBuilder::set_shaders(VkShaderModule vertex_shader, VkShaderModule fragment_shader) -> PipelineBuilder&
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

auto PipelineBuilder::set_cull_mode(VkCullModeFlags cull_mode, VkFrontFace front_face) -> PipelineBuilder&
{
  _rasterization_state.cullMode  = cull_mode;
  _rasterization_state.frontFace = front_face;
  return *this;
}

auto PipelineBuilder::enable_depth_test(VkFormat format) -> PipelineBuilder&
{
  _rendering_info.depthAttachmentFormat = format;
  _depth_stencil_state.depthTestEnable  = VK_TRUE;
  _depth_stencil_state.depthWriteEnable = VK_TRUE;
  _depth_stencil_state.depthCompareOp   = VK_COMPARE_OP_GREATER_OR_EQUAL;
  _depth_stencil_state.maxDepthBounds   = 1.f;
  return *this;
}

} }
