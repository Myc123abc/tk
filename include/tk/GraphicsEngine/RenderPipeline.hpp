//
// Render Pipeline
//
// The render pipeline initially uses shader objects and descriptor buffers.
// If either is unsupported, it falls back to traditional pipelines and descriptor sets with a descriptor pool.
//

#pragma once

#include "DescriptorLayout.hpp"
#include "Shader.hpp"
#include "PipelineLayout.hpp"
#include "vk_extension.hpp"
#include "Pipeline.hpp"
#include "config.hpp"

namespace tk { namespace graphics_engine {

  class RenderPipeline
  {
  public:
    RenderPipeline()  = default;
    ~RenderPipeline() = default;

    void destroy() noexcept;

    template <typename PushConstant>
    void bind(Command const& cmd, PushConstant const& pc)
    {
      if (config()->use_shader_object)
        graphics_engine::vkCmdBindShadersEXT(cmd, _stages.size(), _stages.data(), _vk_shaders.data());
      else
        vkCmdBindPipeline(cmd, _bind_point, _pipeline);

      if (_has_descriptor_layout)
      {
        _descriptor_layout.bind(cmd);
      }

      // TODO: handle no push constant case
      vkCmdPushConstants(cmd, _pipeline_layout, _stage, 0, sizeof(PushConstant), &pc);
    }

    void update() { if (_has_descriptor_layout) _descriptor_layout.update(); }

  private:
    friend class Device;

    /*
     * create a render pipeline
     * @device
     * @push_constant_size
     * @descriptors
     * @descriptor_buffer use for descriptor buffer extentsion
     * @descriptor_layout_tag use for descriptor buffer extentsion
     * @shaders use for shader object extentsion
     * @format use for traditional pipeline
     */
    RenderPipeline(Device&                            device,
                   uint32_t                           push_constant_size,
                   std::vector<DescriptorInfo2> const& descriptors, 
                   Buffer&                            descriptor_buffer, 
                   std::string_view                   descriptor_layout_tag, // FIXME: find way to discard this
                   std::vector<std::pair<VkShaderStageFlagBits, std::string_view>> const& shaders,
                   VkFormat format );
  private:
    bool                               _has_descriptor_layout{};
    bool                               _is_graphics_pipeline{};
    DescriptorLayout                   _descriptor_layout;
    PipelineLayout                     _pipeline_layout;
    std::vector<Shader>                _shaders;
    std::vector<VkShaderEXT>           _vk_shaders;
    std::vector<VkShaderStageFlagBits> _stages;
    VkShaderStageFlags                 _stage{};
    Pipeline                           _pipeline;
    VkPipelineBindPoint                _bind_point{};
  };

}}