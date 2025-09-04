#include "tk/GraphicsEngine/RenderPipeline.hpp"
#include "tk/GraphicsEngine/Device.hpp"
#include "tk/GraphicsEngine/config.hpp"
// FIXME: tmp
#include "tk/ErrorHandling.hpp"

#include <algorithm>

namespace tk { namespace graphics_engine {

void RenderPipeline::destroy() noexcept
{
  if (_has_descriptor_layout)
    _descriptor_layout.destroy();
  _pipeline_layout.destroy();
  if (config()->use_shader_object)
    for (auto& shader : _shaders)
      shader.destroy();
  else
    _pipeline.destroy();
}

RenderPipeline::RenderPipeline(
  Device&                            device,
  uint32_t                           push_constant_size,
  std::vector<DescriptorInfo2> const& descriptors,           // TODO: only use set 0 (single set, multiple sets not process)
  Buffer&                            descriptor_buffer, 
  std::string_view                   descriptor_layout_tag, // FIXME: find way to discard this
  std::vector<std::pair<VkShaderStageFlagBits, std::string_view>> const& shaders,
  VkFormat format)
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

  throw_if(push_constant_size == 0, "[RenderPipeline] unhandle push constant not use case");
  auto pc = VkPushConstantRange
  {
    .stageFlags = _is_graphics_pipeline ? static_cast<VkShaderStageFlags>(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
                                        : static_cast<VkShaderStageFlags>(VK_SHADER_STAGE_COMPUTE_BIT),
    .size       = push_constant_size,
  };

  _bind_point = _is_graphics_pipeline ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE;

  std::vector<VkDescriptorSetLayout> descritptor_layouts;
  if (!descriptors.empty())
  { 
    _has_descriptor_layout = true;

    _descriptor_layout = device.create_descriptor_layout(descriptors);
    
    if (config()->use_descriptor_buffer)
      _descriptor_layout.upload(descriptor_buffer, descriptor_layout_tag);

    descritptor_layouts.emplace_back(_descriptor_layout);
  }

  _pipeline_layout = device.create_pipeline_layout(descritptor_layouts, { pc });

  if (_has_descriptor_layout)
    _descriptor_layout.set(_pipeline_layout, _bind_point);
    
  if (_is_graphics_pipeline)
  {
    if (config()->use_shader_object)
    {
      device.create_shaders(
      {
        { _shaders[0], shaders[0].first, shaders[0].second.data(), descritptor_layouts, { pc } },
        { _shaders[1], shaders[1].first, shaders[1].second.data(), descritptor_layouts, { pc } },
      }, true);
    }
    else
    {
      auto vert = std::find_if(shaders.begin(), shaders.end(), [](auto const& info)
      {
        return info.first == VK_SHADER_STAGE_VERTEX_BIT;
      });
      auto frag = std::find_if(shaders.begin(), shaders.end(), [](auto const& info)
      {
        return info.first == VK_SHADER_STAGE_FRAGMENT_BIT;
      });
      _pipeline = device.create_pipeline(_pipeline_layout, vert->second, frag->second, format);
    }
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

}}