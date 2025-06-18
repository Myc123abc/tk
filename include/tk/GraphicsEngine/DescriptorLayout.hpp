#pragma once

#include "MemoryAllocator.hpp"
#include "CommandPool.hpp"

#include <vulkan/vulkan.h>

#include <vector>

namespace tk { namespace graphics_engine {

  struct DescriptorInfo
  {
    int                 binding = -1;
    VkDescriptorType    type    = VK_DESCRIPTOR_TYPE_MAX_ENUM;
    VkShaderStageFlags  stages  = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
    Image*              image   = {};
    VkSampler           sampler = {};
    uint32_t            count   = 1;
  };

  void bind_descriptor_buffer(Command& cmd, Buffer const& buffer);

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

    void upload(Buffer& buffer, std::string_view tag);

    auto update() -> uint32_t;

    void bind(Command& cmd);

    void set(VkPipelineLayout pipeline_layout, VkPipelineBindPoint bind_point) noexcept
    {
      _pipeline_layout = pipeline_layout;
      _bind_point      = bind_point;
    }

  private:
    friend class Device;

    DescriptorLayout(class Device* device, std::vector<DescriptorInfo> const& layouts);
    
  private:
    class Device*                _device{};
    VkDescriptorSetLayout        _layout{};
    VkDeviceSize                 _size{};
    std::vector<DescriptorInfo>  _descriptors;
    VkPipelineLayout             _pipeline_layout{};
    VkPipelineBindPoint          _bind_point{};
    std::string                  _tag{};
    Buffer*                      _buffer{};
    char*                        _ptr{};
  };

}}