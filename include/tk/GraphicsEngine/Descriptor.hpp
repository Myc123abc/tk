#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace tk { namespace graphics_engine {

  class DescriptorLayout
  {
  public:
    DescriptorLayout()  = default;
    ~DescriptorLayout() = default;

    operator VkDescriptorSetLayout() const noexcept { return _layout; }
    auto get_address() const { return &_layout; }

    void destroy() noexcept;

  private:
    friend class Device;

    DescriptorLayout(VkDevice device, std::vector<VkDescriptorSetLayoutBinding> const& layouts);
    
  private:
    VkDevice              _device = VK_NULL_HANDLE;
    VkDescriptorSetLayout _layout = VK_NULL_HANDLE;
  };

}}