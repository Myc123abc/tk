#pragma once

#include "MemoryAllocator.hpp"
#include "CommandPool.hpp"

#include <vulkan/vulkan.h>

#include <vector>

namespace tk { namespace graphics_engine {

  struct DescriptorInfo
  {
    int                 binding    = -1;
    VkDescriptorType    type       = VK_DESCRIPTOR_TYPE_MAX_ENUM;
    VkShaderStageFlags  stages     = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
    VkImageView         image_view = VK_NULL_HANDLE;
    VkSampler           sampler    = VK_NULL_HANDLE;
    uint32_t            count      = 1;
  };

  void bind_descriptor_buffer(Command& cmd, VkDeviceAddress address, VkBufferUsageFlags usage, VkPipelineLayout layout, VkPipelineBindPoint point);

  class DescriptorLayout
  {
  public:
    DescriptorLayout()  = default;
    ~DescriptorLayout() = default;

    // FIXME: discard, should integrate DescriptorLayout to other class
    operator VkDescriptorSetLayout() const noexcept { return _layout; }
    auto get_address() { return &_layout; }

    void destroy() noexcept;

    auto size() const noexcept { return _size; }

    void update_descriptors(Buffer const& buffer);

    void update_descriptor_image_views(std::vector<std::pair<uint32_t, VkImageView>> const& views);

  private:
    friend class Device;

    DescriptorLayout(class Device* device, std::vector<DescriptorInfo> const& layouts);
    
  private:
    class Device*               _device = nullptr;
    VkDescriptorSetLayout       _layout = VK_NULL_HANDLE;
    VkDeviceSize                _size   = 0;
    std::vector<DescriptorInfo> _descriptors;
  };

}}