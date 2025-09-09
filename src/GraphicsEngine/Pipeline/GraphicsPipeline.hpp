#pragma once

#include "../MemoryAllocator.hpp"
#include "../CommandPool.hpp"

#include <vulkan/vulkan.h>

#include <span>


namespace tk { namespace graphics_engine{

enum class ShaderType
{
  fragment,
};

enum class DescriptorType
{
  sampler2D,
};

struct DescriptorInfo
{
  DescriptorInfo(ShaderType shader_type, uint32_t binding, DescriptorType descriptor_type, std::vector<Image> const& images, VkSampler sampler);

  VkShaderStageFlags shader_type{};
  uint32_t           binding{};
  VkDescriptorType   descriptor_type{};
  std::vector<Image> images{};
  VkSampler          sampler{};
};

struct DescriptorUpdateInfo
{
  DescriptorUpdateInfo(ShaderType shader_type, uint32_t binding, std::vector<Image> const& images);

  VkShaderStageFlags shader_type{};
  uint32_t           binding{};
  std::vector<Image> images{};
};

struct GraphicsPipelineCreateInfo
{
  VkDevice                    device{};
  std::vector<DescriptorInfo> descriptor_infos;
  uint32_t                    push_constant_size{};
  VkFormat                    color_attachment_format{};
  std::string_view            vertex;
  std::string_view            fragment;
};

class GraphicsPipeline
{
public:
  void init(GraphicsPipelineCreateInfo const& create_info);
  void destroy() const noexcept;
  void destroy_without_shader_modules() const noexcept;

  void create_descriptor_set_layout(std::span<DescriptorInfo const> infos);
  void create_descriptor_pool(std::span<DescriptorInfo const> infos);
  void allocate_descriptor_sets(uint32_t variable_descriptor_count);
  void update_descriptor_sets(std::span<DescriptorInfo const> infos);

  void create_pipeline_layout(uint32_t push_constant_size);
  void create_pipeline(VkFormat format);
  auto create_shader_module(std::string_view shader) -> VkShaderModule;

  template <typename PushConstant>
  void bind(Command const& cmd, PushConstant push_constant) const noexcept
  {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline_layout, 0, 1, &_descriptor_set, 0, nullptr);
    vkCmdPushConstants(cmd, _pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push_constant), &push_constant);
  }

  void set_pipeline_state(Command const& cmd, VkExtent2D extent) const noexcept;

  void recreate(std::vector<DescriptorUpdateInfo> const& infos);

private:
  GraphicsPipelineCreateInfo _create_info;
  
  // descriptor resources
  VkDescriptorSetLayout _descriptor_set_layout{};
  VkDescriptorPool      _descriptor_pool{};
  VkDescriptorSet       _descriptor_set{};

  // pipeline resources
  VkPipelineLayout      _pipeline_layout{};
  VkPipeline            _pipeline{};
  VkShaderModule        _vertex_shader_module{};
  VkShaderModule        _fragment_shader_module{};
};

}}