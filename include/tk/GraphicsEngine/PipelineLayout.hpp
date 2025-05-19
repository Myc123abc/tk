//
// pipeline layout
//

#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace tk { namespace graphics_engine {

  class PipelineLayout
  {
  public:
    PipelineLayout()  = default;
    ~PipelineLayout() = default;
    
    operator VkPipelineLayout() const noexcept { return _layout; }

    void destroy();

  private:
    friend class Device;

    PipelineLayout(VkDevice device, std::vector<VkDescriptorSetLayout> const& descritptor_layouts, std::vector<VkPushConstantRange> const& push_constants);

  private:
    VkDevice         _device{};
    VkPipelineLayout _layout{};
  };

}}