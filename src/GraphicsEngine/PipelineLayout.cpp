#include "tk/GraphicsEngine/PipelineLayout.hpp"
#include "tk/ErrorHandling.hpp"

#include <cassert>

namespace tk { namespace graphics_engine {

PipelineLayout::PipelineLayout(VkDevice device, std::vector<VkDescriptorSetLayout> const& descritptor_layouts, std::vector<VkPushConstantRange> const& push_constants)
{
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
}

void PipelineLayout::destroy()
{
  assert(_device && _layout);
  vkDestroyPipelineLayout(_device, _layout, nullptr);
  _device = {};
  _layout = {}; 
}

}}