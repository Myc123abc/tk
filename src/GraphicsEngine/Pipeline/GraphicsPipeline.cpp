#include "tk/GraphicsEngine/Pipeline/GraphicsPipeline.hpp"
#include "tk/ErrorHandling.hpp"
#include "tk/util.hpp"

#include <vulkan/vulkan.h>
#include <vector>
#include <unordered_map>
#include <span>
#include <algorithm>


using namespace tk;
using namespace tk::graphics_engine;

namespace
{

inline void check(bool b, std::string_view msg)
{
  throw_if(b, "[GraphicsPipeline] {}", msg);
}

auto to_vk_type(DescriptorType type)
{
  using enum DescriptorType;
  static std::unordered_map<DescriptorType, VkDescriptorType> types
  {
    { sampler2D, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER },
  };
  auto it = types.find(type);
  check(it == types.end(), "unsupported descriptor type");
  return it->second;
}

auto to_image_layout(VkDescriptorType type)
{
  static std::unordered_map<VkDescriptorType, VkImageLayout> types
  {
    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
  };
  auto it = types.find(type);
  check(it == types.end(), "unsupported descriptor type");
  return it->second;
}

auto to_vk_type(ShaderType type)
{
  using enum ShaderType;
  static std::unordered_map<ShaderType, VkShaderStageFlags> types
  {
    { fragment, VK_SHADER_STAGE_FRAGMENT_BIT },
  };
  auto it = types.find(type);
  check(it == types.end(), "unsupported shader type");
  return it->second;
}

void check_descriptor_infos_valid(std::span<DescriptorInfo const> infos)
{
  check(infos.size() != 1, "only support single sampler2D now");
  check(infos[0].binding != 0, "binding only 0 now"); // TODO: multiple bindings not have duplicate binding number
}

}

namespace tk { namespace graphics_engine {

DescriptorInfo::DescriptorInfo(ShaderType shader_type, uint32_t binding, DescriptorType descriptor_type, std::vector<Image> const& images, VkSampler sampler)
  : shader_type(to_vk_type(shader_type)),
    binding(binding),
    descriptor_type(to_vk_type(descriptor_type)),
    images(images),
    sampler(sampler) {}

DescriptorUpdateInfo::DescriptorUpdateInfo(ShaderType shader_type, uint32_t binding, std::vector<Image> const& images)
  : shader_type(to_vk_type(shader_type)),
    binding(binding),
    images(images) {}

void GraphicsPipeline::init(GraphicsPipelineCreateInfo const& create_info)
{
  _create_info = create_info;
  
  // promise valid
  check_descriptor_infos_valid(create_info.descriptor_infos);

  // create descriptor resources
  create_descriptor_set_layout(create_info.descriptor_infos);
  create_descriptor_pool(create_info.descriptor_infos);
  allocate_descriptor_sets(create_info.descriptor_infos[0].images.size()); // TODO: only variable descriptor
  update_descriptor_sets(create_info.descriptor_infos);

  // create pipeline resources
  create_pipeline_layout(create_info.push_constant_size);
  _vertex_shader_module   = create_shader_module(create_info.vertex);
  _fragment_shader_module = create_shader_module(create_info.fragment);
  create_pipeline(create_info.color_attachment_format);
}

void GraphicsPipeline::destroy() const noexcept
{
  destroy_without_shader_modules();
  vkDestroyShaderModule(_create_info.device, _vertex_shader_module, nullptr);
  vkDestroyShaderModule(_create_info.device, _fragment_shader_module, nullptr);
}

void GraphicsPipeline::destroy_without_shader_modules() const noexcept
{
  vkDestroyDescriptorSetLayout(_create_info.device, _descriptor_set_layout, nullptr);
  vkDestroyDescriptorPool(_create_info.device, _descriptor_pool, nullptr);
  vkDestroyPipelineLayout(_create_info.device, _pipeline_layout, nullptr);
  vkDestroyPipeline(_create_info.device, _pipeline, nullptr);
}

void GraphicsPipeline::set_pipeline_state(Command const& cmd, VkExtent2D extent) const noexcept
{
  auto viewport = VkViewport
  {
    .width  = static_cast<float>(extent.width),
    .height = static_cast<float>(extent.height), 
    .maxDepth = 1.f,
  };
  vkCmdSetViewport(cmd, 0, 1, &viewport);
  VkRect2D scissor{ .extent = extent, };
  vkCmdSetScissor(cmd, 0, 1, &scissor);
}

void GraphicsPipeline::recreate(std::vector<DescriptorUpdateInfo> const& infos)
{
  // update descriptor infos
  for (auto const& info : infos)
  {
    auto it = std::ranges::find_if(_create_info.descriptor_infos, [&](auto const& desc_info)
    {
      return info.shader_type == desc_info.shader_type &&
             info.binding     == desc_info.binding;
    });
    check(it == _create_info.descriptor_infos.end(), "unexist descriptor!");
    it->images = info.images;
  }

  // recreate descriptor resources
  create_descriptor_set_layout(_create_info.descriptor_infos);
  create_descriptor_pool(_create_info.descriptor_infos);
  allocate_descriptor_sets(_create_info.descriptor_infos[0].images.size()); // TODO: only variable descriptor
  update_descriptor_sets(_create_info.descriptor_infos);

  // recreate pipeline resources
  create_pipeline_layout(_create_info.push_constant_size);
  create_pipeline(_create_info.color_attachment_format);
}

////////////////////////////////////////////////////////////////////////////////
///                           Descriptor Resources
////////////////////////////////////////////////////////////////////////////////

void GraphicsPipeline::create_descriptor_set_layout(std::span<DescriptorInfo const> infos)
{
  auto bindings                  = std::vector<VkDescriptorSetLayoutBinding>();
  auto binding_flags             = std::vector<VkDescriptorBindingFlags>();
  auto binding_flags_create_info = VkDescriptorSetLayoutBindingFlagsCreateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };
  auto layout_create_info        = VkDescriptorSetLayoutCreateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
  
  // add descriptor binding info
  for (auto const& info : infos)
  {
    bindings.emplace_back(VkDescriptorSetLayoutBinding
    {
      .binding         = info.binding,
      .descriptorType  = info.descriptor_type,
      .descriptorCount = static_cast<uint32_t>(info.images.size()),
      .stageFlags      = info.shader_type,
    });
    // whether this binding is variable-sized descriptor
    binding_flags.emplace_back(info.images.size() > 1 ? VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT : 0);
  };
  
  // fill binding flags create info
  binding_flags_create_info.bindingCount  = static_cast<uint32_t>(binding_flags.size()),
  binding_flags_create_info.pBindingFlags = binding_flags.data();

  // fill layout create info
  layout_create_info.pBindings    = bindings.data();
  layout_create_info.bindingCount = static_cast<uint32_t>(bindings.size());
  layout_create_info.pNext        = &binding_flags_create_info;
    
  // create descriptor set layout
  throw_if(vkCreateDescriptorSetLayout(_create_info.device, &layout_create_info, nullptr, &_descriptor_set_layout) != VK_SUCCESS,
           "failed to create descriptor set layout");
}

void GraphicsPipeline::create_descriptor_pool(std::span<DescriptorInfo const> infos)
{
  // create descriptor pool
  auto pool_sizes  = std::vector<VkDescriptorPoolSize>();
  auto create_info = VkDescriptorPoolCreateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };

  // add pool size of every descriptor type
  for (auto const& info : infos)
    pool_sizes.emplace_back(VkDescriptorPoolSize
    {
      .type            = info.descriptor_type,
	    .descriptorCount = static_cast<uint32_t>(info.images.size()),
    });

  // fill create info
  create_info.maxSets       = 1, // TODO: only single set now
  create_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
  create_info.pPoolSizes    = pool_sizes.data(),

  // create descriptor pool
  throw_if(vkCreateDescriptorPool(_create_info.device, &create_info, nullptr, &_descriptor_pool) != VK_SUCCESS,
           "failed to create descriptor pool");
}

void GraphicsPipeline::allocate_descriptor_sets(uint32_t variable_descriptor_count)
{
  auto variable_descriptor_counts              = std::vector<uint32_t>();
  auto variable_descriptor_count_allocate_info = VkDescriptorSetVariableDescriptorCountAllocateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO };
  auto descriptor_set_allocate_info            = VkDescriptorSetAllocateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };

  // add variable descriptor counts
  variable_descriptor_counts = { variable_descriptor_count };  // TODO: only single descriptor set's variable descriptor counts using now,
                                                               //       and I dont't know whether all variable-sized descriptors
                                                               //       sharing single variable descriptor counts per descriptor set

  // fill variable count allocate info
  variable_descriptor_count_allocate_info.descriptorSetCount = static_cast<uint32_t>(variable_descriptor_counts.size());
  variable_descriptor_count_allocate_info.pDescriptorCounts  = variable_descriptor_counts.data();

  // fill descriptor set allocate info
  descriptor_set_allocate_info.descriptorPool     = _descriptor_pool;
  descriptor_set_allocate_info.pSetLayouts        = &_descriptor_set_layout;
  descriptor_set_allocate_info.descriptorSetCount = 1; // TODO: single set now
  descriptor_set_allocate_info.pNext              = &variable_descriptor_count_allocate_info;

  // allocate descriptor sets
  throw_if(vkAllocateDescriptorSets(_create_info.device, &descriptor_set_allocate_info, &_descriptor_set) != VK_SUCCESS,
           "failed to allocate descriptor set");
}

void GraphicsPipeline::update_descriptor_sets(std::span<DescriptorInfo const> infos)
{
  auto image_infos = std::vector<VkDescriptorImageInfo>();
  auto writes      = std::vector<VkWriteDescriptorSet>(infos.size());

  // TODO: assume only single descriptor and use combined image
  assert(infos.size() == 1);
  auto& tmp_info = infos[0];

  // resize image infos
  image_infos.resize(tmp_info.images.size());

  // fill image descriptor infos
  for (auto i = 0; i < image_infos.size(); ++i)
  {
    image_infos[i] =
    {
      .sampler     = tmp_info.sampler,
      .imageView   = tmp_info.images[i].view(),
      .imageLayout = to_image_layout(tmp_info.descriptor_type),
    };
  }

  // fill descriptor set writes
  for (auto i = 0; i < infos.size(); ++i)
  {
    auto& info = infos[i];
    writes[i] =
    {
      .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet          = _descriptor_set, // TODO: only single set now
      .dstBinding      = info.binding,
      .descriptorCount = static_cast<uint32_t>(info.images.size()),
      .descriptorType  = info.descriptor_type,
      .pImageInfo      = image_infos.data(), // TODO: assume only single descriptor set and only image
    };
  }

  // update descriptor sets
  vkUpdateDescriptorSets(_create_info.device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

////////////////////////////////////////////////////////////////////////////////
///                           Pipeline Resources
////////////////////////////////////////////////////////////////////////////////

void GraphicsPipeline::create_pipeline_layout(uint32_t push_constant_size)
{
  auto pipeline_layout_create_info = VkPipelineLayoutCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
  auto descriptor_set_layouts      = { _descriptor_set_layout };
  auto push_constant               = VkPushConstantRange{};

  // fill push constant
  push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
  push_constant.size       = push_constant_size;

  // make push constants
  auto push_constants = { push_constant };
  
  // fill pipeline layout create info
  pipeline_layout_create_info.setLayoutCount         = static_cast<uint32_t>(descriptor_set_layouts.size());
  pipeline_layout_create_info.pSetLayouts            = descriptor_set_layouts.begin();
  pipeline_layout_create_info.pushConstantRangeCount = static_cast<uint32_t>(push_constants.size());
  pipeline_layout_create_info.pPushConstantRanges    = push_constants.begin();
  
  // create pipeline layout
  throw_if(vkCreatePipelineLayout(_create_info.device, &pipeline_layout_create_info, nullptr, &_pipeline_layout) != VK_SUCCESS,
           "failed to create pipeline layout");
}

void GraphicsPipeline::create_pipeline(VkFormat format)
{
  VkPipelineRenderingCreateInfo rendering_info
  { 
    .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
    .colorAttachmentCount    = 1,
    .pColorAttachmentFormats = &format,
  };

  std::vector<VkPipelineShaderStageCreateInfo> shader_stages
  {
    {
      .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage  = VK_SHADER_STAGE_VERTEX_BIT,
      .module = _vertex_shader_module,
      .pName  = "main",
    },
    {
      .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = _fragment_shader_module,
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
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
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
    .blendEnable         = VK_TRUE,
    .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
    .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    .colorBlendOp        = VK_BLEND_OP_ADD,
    .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
    .alphaBlendOp        = VK_BLEND_OP_ADD,
    .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT |
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
    .layout              = _pipeline_layout,
  };
  throw_if(vkCreateGraphicsPipelines(_create_info.device, nullptr, 1, &info, nullptr, &_pipeline) != VK_SUCCESS,
           "failed to create pipeline");
}

auto GraphicsPipeline::create_shader_module(std::string_view shader) -> VkShaderModule
{
  VkShaderModule module;

  auto data = util::get_file_data(shader);
  VkShaderModuleCreateInfo info
  {
    .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = data.size() * sizeof(uint32_t),
    .pCode    = reinterpret_cast<uint32_t*>(data.data()),
  };
  throw_if(vkCreateShaderModule(_create_info.device, &info, nullptr, &module) != VK_SUCCESS,
           "failed to create shader from {}", shader);
  return module;
}

}}