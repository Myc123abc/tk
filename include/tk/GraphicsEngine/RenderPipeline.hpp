#pragma once

#include "DescriptorLayout.hpp"
#include "Shader.hpp"
#include "PipelineLayout.hpp"
#include "vk_extension.hpp"

namespace tk { namespace graphics_engine {

  class RenderPipeline
  {
  public:
    RenderPipeline()  = default;
    ~RenderPipeline() = default;

    void destroy()
    {
      if (_has_descriptor_layout)
        _descriptor_layout.destroy();
      _pipeline_layout.destroy();
      for (auto& shader : _shaders)
        shader.destroy();
    }

    template <typename PushConstant>
    void bind(Command& cmd, PushConstant const& pc)
    {
      graphics_engine::vkCmdBindShadersEXT(cmd, _stages.size(), _stages.data(), _vk_shaders.data());
      if (_has_descriptor_layout)
      {
        _descriptor_layout.bind(cmd);
      }
      // TODO: handle no push constant case
      vkCmdPushConstants(cmd, _pipeline_layout, _stage, 0, sizeof(PushConstant), &pc);
    }

    void update() { _descriptor_layout.update(); }

  private:
    friend class Device;
    RenderPipeline(Device&                            device,
                   uint32_t                           push_constant_size,
                   std::vector<DescriptorInfo> const& descriptors, 
                   Buffer&                            descriptor_buffer, 
                   std::string_view                   descriptor_layout_tag, // FIXME: find way to discard this
                   std::vector<std::pair<VkShaderStageFlagBits, std::string_view>> const& shaders);
  private:
    bool                               _has_descriptor_layout{};
    bool                               _is_graphics_pipeline{};
    DescriptorLayout                   _descriptor_layout;
    PipelineLayout                     _pipeline_layout;
    std::vector<Shader>                _shaders;
    std::vector<VkShaderEXT>           _vk_shaders;
    std::vector<VkShaderStageFlagBits> _stages;
    VkShaderStageFlags                 _stage{};
  };

}}