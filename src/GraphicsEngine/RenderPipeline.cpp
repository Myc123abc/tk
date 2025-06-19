#include "tk/GraphicsEngine/RenderPipeline.hpp"
#include "tk/GraphicsEngine/Device.hpp"

// FIXME: tmp
#include "tk/ErrorHandling.hpp"

#include <algorithm>

namespace tk { namespace graphics_engine {

RenderPipeline::RenderPipeline(
  Device&                            device,
  uint32_t                           push_constant_size,
  std::vector<DescriptorInfo> const& descriptors, 
  Buffer&                            descriptor_buffer, 
  std::string_view                   descriptor_layout_tag, // FIXME: find way to discard this
  std::vector<std::pair<VkShaderStageFlagBits, std::string_view>> const& shaders)
{
  auto has_stage = [&](VkShaderStageFlagBits stage)
  {
    return std::ranges::any_of(shaders, [=](auto const& info)
    {
      return info.first == stage;
    });
  };

  // TODO: expand compute pipeline
  _is_graphics_pipeline = has_stage(VK_SHADER_STAGE_VERTEX_BIT) && has_stage(VK_SHADER_STAGE_FRAGMENT_BIT);
  throw_if(!_is_graphics_pipeline, "[RenderPipeline] only support graphic pipeline now");
  // TODO: maybe have graphics pipeline which have vert, frag and comps
  if (_is_graphics_pipeline)
  {
    throw_if(shaders.size() != 2, "[RenderPipeline] only need vertex and fragment shaders in graphic pipeline");
    _shaders.resize(2);
  }

  auto pc = VkPushConstantRange
  {
    .stageFlags = _is_graphics_pipeline ? static_cast<VkShaderStageFlags>(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
                                        : static_cast<VkShaderStageFlags>(VK_SHADER_STAGE_COMPUTE_BIT),
    .size       = push_constant_size,
  };

  if (!descriptors.empty())
  { 
    _has_descriptor_layout = true;

    _descriptor_layout = device.create_descriptor_layout(descriptors);
    _descriptor_layout.upload(descriptor_buffer, descriptor_layout_tag);

    _pipeline_layout = device.create_pipeline_layout({ _descriptor_layout }, { pc });

    _descriptor_layout.set(_pipeline_layout, _is_graphics_pipeline ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE);
    
    if (_is_graphics_pipeline)
    {
      device.create_shaders(
      {
        { _shaders[0], shaders[0].first, shaders[0].second.data(), { _descriptor_layout }, { pc } },
        { _shaders[1], shaders[1].first, shaders[1].second.data(), { _descriptor_layout }, { pc } },
      }, true);
    }

    _vk_shaders.resize(shaders.size());
    _stages.resize(shaders.size());
    for (auto i = 0; i < shaders.size(); ++i)
    {
      _vk_shaders[i] = _shaders[i];
      _stages[i] = shaders[i].first;
      _stage |= shaders[i].first;
    }
  }
  else
  {
    // TODO:
    throw_if(true, "[RenderPipeline] also need to handle no descriptor, no pushconstant cases");
  }
}

}}