#include "tk/GraphicsEngine/Pipeline.hpp"
#include "tk/GraphicsEngine/Device.hpp"
#include "tk/util.hpp"
#include "tk/ErrorHandling.hpp"
#include "tk/GraphicsEngine/config.hpp"

namespace tk { namespace graphics_engine {

void Pipeline::destroy() noexcept
{
  vkDestroyPipeline(_device->get(), _pipeline, nullptr);
}

auto Pipeline::create_shader_module(std::string_view shader) -> VkShaderModule
{
  VkShaderModule module;

  auto data = util::get_file_data(shader);
  VkShaderModuleCreateInfo info
  {
    .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = data.size() * sizeof(uint32_t),
    .pCode    = reinterpret_cast<uint32_t*>(data.data()),
  };
  throw_if(vkCreateShaderModule(_device->get(), &info, nullptr, &module) != VK_SUCCESS,
           "failed to create shader from {}", shader);
  return module;
}


Pipeline::Pipeline(Device& device, VkPipelineLayout layout, std::string_view vert, std::string_view frag, std::vector<VkDescriptorSetLayout> const& descritptor_layouts, std::vector<VkPushConstantRange> const& push_constants, VkFormat format)
  : _device(&device)
{
  _format = format;
  VkPipelineRenderingCreateInfo rendering_info
  { 
    .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
    .colorAttachmentCount    = 1,
    .pColorAttachmentFormats = &_format,
  };

  std::vector<VkPipelineShaderStageCreateInfo> shader_stages
  {
    {
      .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage  = VK_SHADER_STAGE_VERTEX_BIT,
      .module = create_shader_module(vert),
      .pName  = "main",
    },
    {
      .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = create_shader_module(frag),
      .pName  = "main",
    },
  };

  VkPipelineVertexInputStateCreateInfo vertex_input_state
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
  };

  VkPipelineInputAssemblyStateCreateInfo input_assembly_state
  { 
    .sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, // TODO: change to list
  };

  VkPipelineViewportStateCreateInfo viewport_state
  { 
    .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .scissorCount  = 1,
  };

  VkPipelineRasterizationStateCreateInfo rasterization_state
  {
    .sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .cullMode    = VK_CULL_MODE_BACK_BIT,
    .frontFace   = VK_FRONT_FACE_CLOCKWISE,
    .lineWidth   = 1.f,
  };

  VkPipelineMultisampleStateCreateInfo multisample_state
  { 
    .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    .minSampleShading     = 1.f,
  };

  VkPipelineDepthStencilStateCreateInfo depth_stencil_state
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
  };

  VkPipelineColorBlendAttachmentState color_blend_attachment
  {
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                      VK_COLOR_COMPONENT_G_BIT |
                      VK_COLOR_COMPONENT_B_BIT |
                      VK_COLOR_COMPONENT_A_BIT,
  };
  VkPipelineColorBlendStateCreateInfo color_blend_state
  { 
    .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .attachmentCount = 1,
    .pAttachments    = &color_blend_attachment,
  };

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

  VkGraphicsPipelineCreateInfo info
  {
    .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .pNext               = &rendering_info,
    .stageCount          = static_cast<uint32_t>(shader_stages.size()),
    .pStages             = shader_stages.data(),
    .pVertexInputState   = &vertex_input_state,
    .pInputAssemblyState = &input_assembly_state,
    .pTessellationState  = nullptr,
    .pViewportState      = &viewport_state,
    .pRasterizationState = &rasterization_state,
    .pMultisampleState   = &multisample_state,
    .pDepthStencilState  = &depth_stencil_state,
    .pColorBlendState    = &color_blend_state,
    .pDynamicState       = &dynamic_state,
    .layout              = layout,
  };
  if (config()->use_descriptor_buffer)
    info.flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
  throw_if(vkCreateGraphicsPipelines(device, nullptr, 1, &info, nullptr, &_pipeline) != VK_SUCCESS,
           "failed to create pipeline");

  for (auto shader : shader_stages)
    vkDestroyShaderModule(device, shader.module, nullptr);
}

}}